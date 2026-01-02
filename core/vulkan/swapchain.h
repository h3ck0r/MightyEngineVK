
#ifndef VK_INIT_SWAPCHAIN_H_
#define VK_INIT_SWAPCHAIN_H_

#include "device.h"
#include "surface.h"
#include "vulkan/vulkan.hpp"

namespace engine_init {

struct Swapchain {
   public:
    Swapchain(const Device& device,
        const Surface& surface,
        const QueueFamily& queue_family);
    // No copy
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    vk::SwapchainKHR swapchain() const { return swapchain_.get(); }

   private:
    void CreateSwapchain(const vk::Device& device,
        const vk::SurfaceKHR& surface,
        uint32_t queue_family_index);
    vk::UniqueSwapchainKHR swapchain_;
    std::vector<vk::Image> swapchain_images_;
};
}  // namespace engine_init

#endif