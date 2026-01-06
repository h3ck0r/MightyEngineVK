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
#include "vulkan/window_handle.h"

namespace engine_init {
Context::Context() {
    InitVulkan();
}
void Context::InitVulkan() {
    window_handle = std::make_unique<WindowHandle>();
    instance = std::make_unique<Instance>();
    debug_messenger = std::make_unique<DebugMessenger>(*instance);
    surface = std::make_unique<Surface>(*instance, *window_handle);
    physical_device = std::make_unique<PhysicalDevice>(*instance);
    queue_family =
        std::make_unique<QueueFamily>(*physical_device, *surface);
    device = std::make_unique<Device>(*physical_device, *queue_family);
    queue = std::make_unique<Queue>(*device, *queue_family);
    swapchain =
        std::make_unique<Swapchain>(*device, *surface, *queue_family);
    command_pool = std::make_unique<CommandPool>(*device, *queue_family);
    descriptor_pool = std::make_unique<DescriptorPool>(*device);

    std::cout << "Finished initializing engine.";
}

void Context::OneTimeSubmit(
    const std::function<void(vk::CommandBuffer)>& func) const {
    vk::CommandBufferAllocateInfo command_buffer_info;
    command_buffer_info.setCommandPool(command_pool->command_pool());
    command_buffer_info.setCommandBufferCount(1);

    vk::UniqueCommandBuffer command_buffer = std::move(device->device()
            .allocateCommandBuffersUnique(command_buffer_info)
            .front());
    command_buffer->begin(
        {vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    func(*command_buffer);
    command_buffer->end();

    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(*command_buffer);
    queue->queue().submit(submitInfo);
    queue->queue().waitIdle();
}

vk::AccessFlags Context::ToAccessFlags(vk::ImageLayout layout) {
    switch (layout) {
        case vk::ImageLayout::eTransferSrcOptimal:
            return vk::AccessFlagBits::eTransferRead;
        case vk::ImageLayout::eTransferDstOptimal:
            return vk::AccessFlagBits::eTransferWrite;
        default:
            return {};
    }
}

uint32_t Context::FindMemoryType(uint32_t type_filter,
    vk::MemoryPropertyFlags properties) const {
    vk::PhysicalDeviceMemoryProperties mem_properties =
        physical_device->physical_device().getMemoryProperties();
    for (uint32_t i = 0; i != mem_properties.memoryTypeCount; ++i) {
        if ((type_filter & (1 << i))
            && (mem_properties.memoryTypes[i].propertyFlags & properties)
                   == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type");
};
}  // namespace engine_init