#ifndef VK_IMAGE_H_
#define VK_IMAGE_H_

#include <vulkan/vulkan.hpp>

#include "../vulkan/context.h"

namespace engine {

struct Image {
   public:
    Image(const engine_init::Context& context,
        vk::Extent2D extent,
        vk::Format format,
        vk::ImageUsageFlags usage);
    // No copy
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    static vk::AccessFlags ToAccessFlags(vk::ImageLayout layout);

    void SetImageLayout(vk::CommandBuffer commandBuffer,
        vk::Image image,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout);

    void CopyImage(vk::CommandBuffer command_buffer,
        vk::Image src_image,
        vk::Image dst_image);

   private:
    vk::UniqueImage image_;
    vk::UniqueImageView view_;
    vk::UniqueDeviceMemory memory_;
    vk::DescriptorImageInfo desc_image_info_;
};
}  // namespace engine

#endif