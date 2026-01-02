#include "physical_device.h"

#include <vulkan/vulkan_core.h>

namespace engine_init {

PhysicalDevice::PhysicalDevice(const Instance& instance) {
    PickPhysicalDevice(instance.instance());
}

void PhysicalDevice::PickPhysicalDevice(const vk::Instance& instance) {
    // TODO: pick best device, not the first one
    physical_device_ = instance.enumeratePhysicalDevices().front();
}

}  // namespace engine_init