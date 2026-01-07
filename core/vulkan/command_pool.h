
#ifndef VK_INIT_COMMAND_POOL_H_
#define VK_INIT_COMMAND_POOL_H_

#include "vulkan/queue_family.h"
#include "vulkan/vulkan.hpp"

namespace engine_lib {

struct CommandPool {
   public:
    CommandPool(const vk::Device& device, const QueueFamily& queue_family);
    // No copy
    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;
    vk::CommandPool command_pool() const { return command_pool_.get(); }

   private:
    void CreateCommandPool(const vk::Device& device, uint32_t queue_family_index);
    vk::UniqueCommandPool command_pool_;
};
}  // namespace engine_lib

#endif