#include "context.h"

#include <functional>
#include <iostream>
#include <memory>

#include "vulkan/command_pool.h"
#include "vulkan/context.h"
#include "vulkan/debug_messenger.h"
#include "vulkan/descriptor_pool.h"
#include "vulkan/device.h"
#include "vulkan/instance.h"
#include "vulkan/physical_device.h"
#include "vulkan/queue.h"
#include "vulkan/queue_family.h"
#include "vulkan/surface.h"
#include "vulkan/swapchain.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/window_handle.h"

namespace engine {
Context::Context() {
    InitVulkan();
}
void Context::InitVulkan() {
    window_handle_ = std::make_unique<engine_lib::WindowHandle>();
    instance_ = std::make_unique<engine_lib::Instance>();
    debug_messenger_ = std::make_unique<engine_lib::DebugMessenger>(instance_handle());
    surface_ = std::make_unique<engine_lib::Surface>(instance_handle(), window_handle());
    physical_device_ = std::make_unique<engine_lib::PhysicalDevice>(instance_handle());
    queue_family_ =
        std::make_unique<engine_lib::QueueFamily>(physical_device(), surface_handle());
    device_ =
        std::make_unique<engine_lib::Device>(physical_device(), queue_family_handle());
    queue_ = std::make_unique<engine_lib::Queue>(device(), queue_family_handle());
    swapchain_ = std::make_unique<engine_lib::Swapchain>(device(),
        surface_handle(),
        queue_family_handle());
    command_pool_ =
        std::make_unique<engine_lib::CommandPool>(device(), queue_family_handle());

    descriptor_pool_ = std::make_unique<engine_lib::DescriptorPool>(*device_);

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

}  // namespace engine