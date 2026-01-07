
#ifndef VK_BUFFER_H_
#define VK_BUFFER_H_

#include <vulkan/vulkan.hpp>

namespace engine {
struct Buffer {
   public:
    enum class Type {
        Scratch,
        AccelInput,
        AccelStorage,
        ShaderBindingTable,
    };
    Buffer(const vk::Device& device,
        const vk::PhysicalDevice& physical_device,
        Type type,
        vk::DeviceSize size,
        const void* data = nullptr);
    Buffer(Buffer&&) noexcept = default;
    Buffer& operator=(Buffer&&) noexcept = default;

    vk::DeviceMemory memory() const { return memory_.get(); }
    vk::Buffer buffer() const { return buffer_.get(); }
    uint64_t device_address() const { return device_address_; }
    const vk::DescriptorBufferInfo& desc_buffer_info() const { return desc_buffer_info_; }

   private:
    void CreateBuffer(const vk::Device& device,
        const vk::PhysicalDevice& physical_device,
        Type type,
        vk::DeviceSize size,
        const void* data = nullptr);

    vk::UniqueBuffer buffer_;
    vk::UniqueDeviceMemory memory_;
    vk::DescriptorBufferInfo desc_buffer_info_;
    uint64_t device_address_ = 0;
};
}  // namespace engine

#endif