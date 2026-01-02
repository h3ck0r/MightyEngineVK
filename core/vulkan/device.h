
#ifndef VK_INIT_DEVICE_H_
#define VK_INIT_DEVICE_H_

#include <vulkan/vulkan.hpp>

#include "queue_family.h"
#include "vulkan/vulkan.hpp"

namespace engine_init {

struct Device {
   public:
    Device(const PhysicalDevice& physical_device,
        const QueueFamily& queue_family);
    // No copy
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    vk::Device device() const { return device_.get(); }

   private:
    void CreateDevice(const vk::PhysicalDevice& physical_deivce,
        const size_t& queue_family_index);
    bool CheckDeviceExtensionSupport(
        const vk::PhysicalDevice& physical_deivce,
        const std::vector<const char*>& required_extensions) const;
    vk::UniqueDevice device_;
};
}  // namespace engine_init

#endif