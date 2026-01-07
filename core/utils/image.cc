#include "image.h"

#include <vulkan/vulkan_core.h>

#include "utils/object_utils.h"

namespace engine {

Image::Image(const engine::Context& context,
    vk::Extent2D extent,
    vk::Format format,
    vk::ImageUsageFlags usage) {
    // Create image
    vk::ImageCreateInfo image_info;
    image_info.setImageType(vk::ImageType::e2D);
    image_info.setExtent({extent.width, extent.height, 1});
    image_info.setMipLevels(1);
    image_info.setArrayLayers(1);
    image_info.setFormat(format);
    image_info.setUsage(usage);
    image_ = context.device().createImageUnique(image_info);

    // Allocate memory
    vk::MemoryRequirements requirements =
        context.device().getImageMemoryRequirements(*image_);
    uint32_t memory_type_index = FindMemoryType(requirements.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        context.physical_device().getMemoryProperties());
    vk::MemoryAllocateInfo memory_info;
    memory_info.setAllocationSize(requirements.size);
    memory_info.setMemoryTypeIndex(memory_type_index);
    memory_ = context.device().allocateMemoryUnique(memory_info);

    // Bind memory and image
    context.device().bindImageMemory(*image_, *memory_, 0);

    // Create image view
    vk::ImageViewCreateInfo image_view_info;
    image_view_info.setImage(*image_);
    image_view_info.setViewType(vk::ImageViewType::e2D);
    image_view_info.setFormat(format);
    image_view_info.setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    view_ = context.device().createImageViewUnique(image_view_info);

    // Set image info
    desc_image_info_.setImageView(*view_);
    desc_image_info_.setImageLayout(vk::ImageLayout::eGeneral);
    context.OneTimeSubmit([&](vk::CommandBuffer command_buffer) {  //
        SetImageLayout(command_buffer,
            *image_,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral);
    });
}

vk::AccessFlags Image::ToAccessFlags(vk::ImageLayout layout) {
    switch (layout) {
        case vk::ImageLayout::eTransferSrcOptimal:
            return vk::AccessFlagBits::eTransferRead;
        case vk::ImageLayout::eTransferDstOptimal:
            return vk::AccessFlagBits::eTransferWrite;
        default:
            return {};
    }
}

void Image::CopyImage(vk::CommandBuffer command_buffer,
    vk::Image src_image,
    vk::Image dst_image) {
    vk::ImageCopy copy_region;
    copy_region.setSrcSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
    copy_region.setDstSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
    copy_region.setExtent({WIDTH, HEIGHT, 1});
    command_buffer.copyImage(src_image,
        vk::ImageLayout::eTransferSrcOptimal,
        dst_image,
        vk::ImageLayout::eTransferDstOptimal,
        copy_region);
}

void Image::SetImageLayout(vk::CommandBuffer command_buffer,
    vk::Image image,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout) {
    vk::ImageMemoryBarrier barrier;
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setImage(image);
    barrier.setOldLayout(old_layout);
    barrier.setNewLayout(new_layout);
    barrier.setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    barrier.setSrcAccessMask(ToAccessFlags(old_layout));
    barrier.setDstAccessMask(ToAccessFlags(new_layout));
    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
        vk::PipelineStageFlagBits::eAllCommands,
        {},
        {},
        {},
        barrier);
}

}  // namespace engine
