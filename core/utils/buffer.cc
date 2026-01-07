#include "buffer.h"

#include <vulkan/vulkan_core.h>

#include "utils/object_utils.h"

namespace engine {

Buffer::Buffer(const vk::Device& device,
    const vk::PhysicalDevice& physical_device,
    Type type,
    vk::DeviceSize size,
    const void* data) {
    CreateBuffer(device, physical_device, type, size, data);
}

void Buffer::CreateBuffer(const vk::Device& device,
    const vk::PhysicalDevice& physical_device,
    Type type,
    vk::DeviceSize size,
    const void* data) {
    vk::BufferUsageFlags usage;
    vk::MemoryPropertyFlags memory_props;

    using Usage = vk::BufferUsageFlagBits;
    using Memory = vk::MemoryPropertyFlagBits;

    if (type == Type::AccelInput) {
        usage = Usage::eAccelerationStructureBuildInputReadOnlyKHR | Usage::eStorageBuffer
                | Usage::eShaderDeviceAddress;
        memory_props = Memory::eHostVisible | Memory::eHostCoherent;
    } else if (type == Type::Scratch) {
        usage = Usage::eStorageBuffer | Usage::eShaderDeviceAddress;
        memory_props = Memory::eDeviceLocal;
    } else if (type == Type::AccelStorage) {
        usage = Usage::eAccelerationStructureStorageKHR | Usage::eShaderDeviceAddress;
        memory_props = Memory::eDeviceLocal;
    } else if (type == Type::ShaderBindingTable) {
        usage = Usage::eShaderBindingTableKHR | Usage::eShaderDeviceAddress;
        memory_props = Memory::eHostVisible | Memory::eHostCoherent;
    }

    buffer_ = device.createBufferUnique({{}, size, usage});

    // Allocate memory
    vk::MemoryRequirements requirements = device.getBufferMemoryRequirements(*buffer_);
    uint32_t memory_type_index = FindMemoryType(requirements.memoryTypeBits,
        memory_props,
        physical_device.getMemoryProperties());

    vk::MemoryAllocateFlagsInfo flags_info{vk::MemoryAllocateFlagBits::eDeviceAddress};

    vk::MemoryAllocateInfo memory_info;
    memory_info.setAllocationSize(requirements.size);
    memory_info.setMemoryTypeIndex(memory_type_index);
    memory_info.setPNext(&flags_info);
    memory_ = device.allocateMemoryUnique(memory_info);

    device.bindBufferMemory(*buffer_, *memory_, 0);

    // Get device address
    vk::BufferDeviceAddressInfoKHR buffer_device_address_info{*buffer_};
    device_address_ = device.getBufferAddressKHR(&buffer_device_address_info);

    desc_buffer_info_.setBuffer(*buffer_);
    desc_buffer_info_.setOffset(0);
    desc_buffer_info_.setRange(size);

    if (data) {
        void* mapped = device.mapMemory(*memory_, 0, size);
        memcpy(mapped, data, size);
        device.unmapMemory(*memory_);
    }
}

}  // namespace engine