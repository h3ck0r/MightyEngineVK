#include "swapchain.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>

#include "device.h"
#include "queue_family.h"
#include "vulkan/vulkan.hpp"

namespace engine_init {

Swapchain::Swapchain(const Device& device,
    const Surface& surface,
    const QueueFamily& queue_family) {
    CreateSwapchain(device.device(),
        surface.surface(),
        queue_family.queue_family_index());
}

void Swapchain::CreateSwapchain(const vk::Device& device,
    const vk::SurfaceKHR& surface,
    uint32_t queue_family_index) {
    vk::SwapchainCreateInfoKHR swapchain_info;
    swapchain_info.setSurface(surface);
    swapchain_info.setMinImageCount(3);
    swapchain_info.setImageFormat(vk::Format::eB8G8R8A8Unorm);
    swapchain_info.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear);
    swapchain_info.setImageExtent({WIDTH, HEIGHT});
    swapchain_info.setImageArrayLayers(1);
    swapchain_info.setImageUsage(vk::ImageUsageFlagBits::eTransferDst);
    swapchain_info.setPreTransform(
        vk::SurfaceTransformFlagBitsKHR::eIdentity);
    swapchain_info.setPresentMode(vk::PresentModeKHR::eFifo);
    swapchain_info.setClipped(true);
    swapchain_info.setQueueFamilyIndices(queue_family_index);
    swapchain_ = device.createSwapchainKHRUnique(swapchain_info);

    swapchain_images_ = device.getSwapchainImagesKHR(*swapchain_);
}

}  // namespace engine_init