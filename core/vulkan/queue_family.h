
#ifndef VK_INIT_QUEUE_FAMILY_H_
#define VK_INIT_QUEUE_FAMILY_H_

#include <cstddef>
#include <vulkan/vulkan.hpp>

#include "physical_device.h"
#include "surface.h"

namespace engine_init {

struct QueueFamily {
   public:
    QueueFamily(const PhysicalDevice& physical_device,
        const Surface& surface);
    // No copy
    QueueFamily(const QueueFamily&) = delete;
    QueueFamily& operator=(const QueueFamily&) = delete;
    size_t queue_family_index() const { return queue_family_index_; }

   private:
    void PickQueueFamilyIndex(const vk::PhysicalDevice& physical_device,
        const vk::SurfaceKHR& surface);
    uint32_t queue_family_index_;
};
}  // namespace engine_init

#endif