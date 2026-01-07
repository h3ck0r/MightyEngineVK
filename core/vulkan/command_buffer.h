
#ifndef VK_INIT_COMMAND_BUFFER_H_
#define VK_INIT_COMMAND_BUFFER_H_

#include <cstdint>

#include "vulkan/command_pool.h"
#include "vulkan/vulkan.hpp"

namespace engine_init {

struct CommandBuffer {
   public:
    CommandBuffer(const vk::Device& device,
        const CommandPool& command_pool,
        uint32_t swapchain_images_count);
    // No copy
    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;
    vk::CommandBuffer get_buffer(uint32_t index) const {
        return command_buffers_[index].get();
    }

   private:
    void CreateCommandBuffer(const vk::Device& device,
        const vk::CommandPool& command_pool,
        uint32_t swapchain_images_count);
    std::vector<vk::UniqueCommandBuffer> command_buffers_;
};
}  // namespace engine_init

#endif