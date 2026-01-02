#include "queue.h"

#include <vulkan/vulkan_core.h>

#include "vulkan/vulkan.hpp"

namespace engine_init {

Queue::Queue(const Device& device, const QueueFamily& queue_family) {
    PickQueue(device.device(), queue_family.queue_family_index());
}

void Queue::PickQueue(const vk::Device& device,
    size_t queue_family_index) {
    queue_ = device.getQueue(queue_family_index, 0);
}

}  // namespace engine_init