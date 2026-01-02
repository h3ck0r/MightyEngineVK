
#ifndef VK_INIT_QUEUE_H_
#define VK_INIT_QUEUE_H_

#include <vulkan/vulkan.hpp>

#include "device.h"

namespace engine_init {

struct Queue {
   public:
    Queue(const Device& device, const QueueFamily& queue_family);
    // No copy
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;
    vk::Queue queue() const { return queue_; }

   private:
    void PickQueue(const vk::Device& device, size_t queue_family_index);
    vk::Queue queue_;
};
}  // namespace engine_init

#endif