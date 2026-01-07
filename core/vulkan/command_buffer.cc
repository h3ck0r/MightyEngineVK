#include "command_buffer.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>

#include "vulkan/vulkan.hpp"

namespace engine_init {

CommandBuffer::CommandBuffer(const vk::Device& device,
    const CommandPool& command_pool,
    uint32_t swapchain_images_count) {
    CreateCommandBuffer(device, command_pool.command_pool(), swapchain_images_count);
}

void CommandBuffer::CreateCommandBuffer(const vk::Device& device,
    const vk::CommandPool& command_pool,
    uint32_t swapchain_images_count) {
    vk::CommandBufferAllocateInfo command_buffer_info;
    command_buffer_info.setCommandPool(command_pool);
    command_buffer_info.setCommandBufferCount(
        static_cast<uint32_t>(swapchain_images_count));
    command_buffers_ = device.allocateCommandBuffersUnique(command_buffer_info);
}

}  // namespace engine_init