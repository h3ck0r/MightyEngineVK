#include "descriptor_pool.h"

#include "vulkan/vulkan.hpp"

namespace engine_init {

DescriptorPool::DescriptorPool(const Device& device) {
    CreateDescriptorPool(device.device());
}

void DescriptorPool::CreateDescriptorPool(const vk::Device& device) {
    vk::DescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.setPoolSizes(pool_sizes);
    descriptor_pool_info.setMaxSets(1);
    descriptor_pool_info.setFlags(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    descriptor_pool_ =
        device.createDescriptorPoolUnique(descriptor_pool_info);
}

}  // namespace engine_init