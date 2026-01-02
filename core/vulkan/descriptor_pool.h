
#ifndef VK_INIT_DESCRIPTOR_POOL_H_
#define VK_INIT_DESCRIPTOR_POOL_H_

#include <vulkan/vulkan.hpp>

#include "device.h"

namespace engine_init {

struct DescriptorPool {
   public:
    DescriptorPool(const Device& device);
    // No copy
    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;
    vk::DescriptorPool descriptor_pool() const {
        return descriptor_pool_.get();
    }

   private:
    void CreateDescriptorPool(const vk::Device& device);
    vk::UniqueDescriptorPool descriptor_pool_;
    std::vector<vk::DescriptorPoolSize> pool_sizes{
        {vk::DescriptorType::eAccelerationStructureKHR, 1},
        {vk::DescriptorType::eStorageImage, 1},
        {vk::DescriptorType::eStorageBuffer, 3},
    };
};
}  // namespace engine_init

#endif