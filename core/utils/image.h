#ifndef VK_IMAGE_H_
#define VK_IMAGE_H_

#include <vulkan/vulkan.hpp>

#include "../vulkan/context.h"
#include "vulkan/vulkan.hpp"

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

    static void SetImageLayout(vk::CommandBuffer command_buffer,
        vk::Image image,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout);

    static void CopyImage(vk::CommandBuffer command_buffer,
        vk::Image src_image,
        vk::Image dst_image);
    const vk::DescriptorImageInfo& desc_image_info() const { return desc_image_info_; }

    vk::Image image() const { return image_.get(); }

   private:
    vk::UniqueImage image_;
    vk::UniqueImageView view_;
    vk::UniqueDeviceMemory memory_;
    vk::DescriptorImageInfo desc_image_info_;
};
}  // namespace engine

#endif