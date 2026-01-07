
#ifndef VK_INIT_SWAPCHAIN_H_
#define VK_INIT_SWAPCHAIN_H_

#include "vulkan/queue_family.h"
#define MAX_IMAGE_IN_FLIGHT 3

#include "surface.h"
#include "vulkan/vulkan.hpp"

namespace engine_init {

struct Swapchain {
   public:
    Swapchain(const vk::Device& device,
        const Surface& surface,
        const QueueFamily& queue_family);
    // No copy
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    const vk::SwapchainKHR& swapchain() const { return swapchain_.get(); }
    std::vector<vk::Image> swapchain_images() const { return swapchain_images_; }
    vk::Image get_swapchain_image(uint32_t index) const {
        return swapchain_images_[index];
    }

   private:
    void CreateSwapchain(const vk::Device& device,
        const vk::SurfaceKHR& surface,
        uint32_t queue_family_index);
    vk::UniqueSwapchainKHR swapchain_;
    std::vector<vk::Image> swapchain_images_;
};
}  // namespace engine_init

#endif