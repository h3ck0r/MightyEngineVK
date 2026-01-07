#include "context.h"

#include <functional>
#include <iostream>
#include <memory>

#include "command_pool.h"
#include "descriptor_pool.h"
#include "surface.h"
#include "vulkan/context.h"
#include "vulkan/debug_messenger.h"
#include "vulkan/device.h"
#include "vulkan/instance.h"
#include "vulkan/queue.h"
#include "vulkan/queue_family.h"
#include "vulkan/swapchain.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/window_handle.h"

namespace engine_init {
Context::Context() {
    InitVulkan();
}
void Context::InitVulkan() {
    window_handle_ = std::make_unique<WindowHandle>();
    instance_ = std::make_unique<Instance>();
    debug_messenger_ = std::make_unique<DebugMessenger>(instance_handle());
    surface_ = std::make_unique<Surface>(instance_handle(), window_handle());
    physical_device_ = std::make_unique<PhysicalDevice>(instance_handle());
    queue_family_ =
        std::make_unique<QueueFamily>(physical_device_handle(), surface_handle());
    device_ = std::make_unique<Device>(physical_device_handle(), queue_family_handle());
    queue_ = std::make_unique<Queue>(device(), queue_family_handle());
    swapchain_ =
        std::make_unique<Swapchain>(device(), surface_handle(), queue_family_handle());
    command_pool_ = std::make_unique<CommandPool>(device(), queue_family_handle());

    descriptor_pool_ = std::make_unique<DescriptorPool>(*device_);

    std::cout << "Finished initializing context for engine.\n";
}

vk::UniqueDescriptorSet Context::AllocateDescSet(vk::DescriptorSetLayout desc_set_layout,
    const vk::DescriptorPool desc_pool,
    vk::Device device) {
    vk::DescriptorSetAllocateInfo desc_set_info;
    desc_set_info.setDescriptorPool(desc_pool);
    desc_set_info.setSetLayouts(desc_set_layout);
    return std::move(device.allocateDescriptorSetsUnique(desc_set_info).front());
}

void Context::OneTimeSubmit(const std::function<void(vk::CommandBuffer)>& func) const {
    vk::CommandBufferAllocateInfo command_buffer_info;
    command_buffer_info.setCommandPool(command_pool_handle().command_pool());
    command_buffer_info.setCommandBufferCount(1);

    vk::UniqueCommandBuffer command_buffer =
        std::move(device().allocateCommandBuffersUnique(command_buffer_info).front());
    command_buffer->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    func(*command_buffer);
    command_buffer->end();

    vk::SubmitInfo submit_info;
    submit_info.setCommandBuffers(*command_buffer);
    queue().submit(submit_info);
    queue().waitIdle();
}

uint32_t Context::FindMemoryType(uint32_t type_filter,
    vk::MemoryPropertyFlags properties) const {
    vk::PhysicalDeviceMemoryProperties mem_properties =
        physical_device_handle().physical_device().getMemoryProperties();
    for (uint32_t i = 0; i != mem_properties.memoryTypeCount; ++i) {
        if ((type_filter & (1 << i))
            && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("Failed to find suitable memory type.");
};
}  // namespace engine_init