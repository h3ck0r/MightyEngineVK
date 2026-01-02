#include "command_pool.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>

#include "device.h"
#include "queue_family.h"
#include "vulkan/vulkan.hpp"

namespace engine_init {

CommandPool::CommandPool(const Device& device,
    const QueueFamily& queue_family) {
    CreateCommandPool(device.device(), queue_family.queue_family_index());
}

void CommandPool::CreateCommandPool(const vk::Device& device,
    uint32_t queue_family_index) {
    vk::CommandPoolCreateInfo command_pool_info;
    command_pool_info.setFlags(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    command_pool_info.setQueueFamilyIndex(queue_family_index);
    command_pool_ = device.createCommandPoolUnique(command_pool_info);
}

}  // namespace engine_init