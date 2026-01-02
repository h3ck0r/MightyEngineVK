#include "queue_family.h"

#include <vulkan/vulkan_core.h>

namespace engine_init {

QueueFamily::QueueFamily(const PhysicalDevice& physical_device,
    const Surface& surface) {
    PickQueueFamilyIndex(physical_device.physical_device(),
        surface.surface());
}

void QueueFamily::PickQueueFamilyIndex(
    const vk::PhysicalDevice& physical_device,
    const vk::SurfaceKHR& surface) {
    // Find queue family
    std::vector queue_families =
        physical_device.getQueueFamilyProperties();
    for (size_t i = 0; i < queue_families.size(); i++) {
        auto support_compute =
            queue_families[i].queueFlags & vk::QueueFlagBits::eCompute;
        auto support_present =
            physical_device.getSurfaceSupportKHR(i, surface);
        if (support_compute && support_present) {
            queue_family_index_ = i;
        }
    }
}

}  // namespace engine_init