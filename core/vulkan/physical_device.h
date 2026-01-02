
#ifndef VK_INIT_PHYSICAL_DEVICE_H_
#define VK_INIT_PHYSICAL_DEVICE_H_

#include <vulkan/vulkan.hpp>

#include "instance.h"

namespace engine_init {

struct PhysicalDevice {
   public:
    PhysicalDevice(const Instance& instance);
    // No copy
    PhysicalDevice(const PhysicalDevice&) = delete;
    PhysicalDevice& operator=(const PhysicalDevice&) = delete;
    vk::PhysicalDevice physical_device() const { return physical_device_; }

   private:
    void PickPhysicalDevice(const vk::Instance& instance);
    vk::PhysicalDevice physical_device_;
};
}  // namespace engine_init

#endif