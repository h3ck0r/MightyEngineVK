#include "command_pool.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>

#include "queue_family.h"
#include "vulkan/vulkan.hpp"

namespace engine_lib {

CommandPool::CommandPool(const vk::Device& device, const QueueFamily& queue_family) {
    CreateCommandPool(device, queue_family.queue_family_index());
}

void CommandPool::CreateCommandPool(const vk::Device& device,
    uint32_t queue_family_index) {
    vk::CommandPoolCreateInfo command_pool_info;
    command_pool_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    command_pool_info.setQueueFamilyIndex(queue_family_index);
    command_pool_ = device.createCommandPoolUnique(command_pool_info);
}

}  // namespace engine_lib