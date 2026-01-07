#include "accel.h"

#include <memory>

namespace engine {
Accel::Accel(const engine::Context& context,
    vk::AccelerationStructureGeometryKHR geometry,
    uint32_t primitive_count,
    vk::AccelerationStructureTypeKHR type) {
    vk::AccelerationStructureBuildGeometryInfoKHR build_geometry_info;
    build_geometry_info.setType(type);
    build_geometry_info.setFlags(
        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
    build_geometry_info.setGeometries(geometry);

    // Create buffer
    vk::AccelerationStructureBuildSizesInfoKHR build_sizes_info =
        context.device().getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice,
            build_geometry_info,
            primitive_count);
    vk::DeviceSize size = build_sizes_info.accelerationStructureSize;
    buffer_ = std::make_unique<Buffer>(context.device(),
        context.physical_device(),
        Buffer::Type::AccelStorage,
        size);

    // Create accel
    vk::AccelerationStructureCreateInfoKHR accel_info;
    accel_info.setBuffer(buffer_->buffer());
    accel_info.setSize(size);
    accel_info.setType(type);
    accel_ = context.device().createAccelerationStructureKHRUnique(accel_info);

    // Build
    Buffer scratch_buffer{context.device(),
        context.physical_device(),
        Buffer::Type::Scratch,
        build_sizes_info.buildScratchSize};
    build_geometry_info.setScratchData(scratch_buffer.device_address());
    build_geometry_info.setDstAccelerationStructure(*accel_);

    context.OneTimeSubmit([&](vk::CommandBuffer command_buffer) {  //
        vk::AccelerationStructureBuildRangeInfoKHR build_range_info;
        build_range_info.setPrimitiveCount(primitive_count);
        build_range_info.setFirstVertex(0);
        build_range_info.setPrimitiveOffset(0);
        build_range_info.setTransformOffset(0);
        command_buffer.buildAccelerationStructuresKHR(build_geometry_info,
            &build_range_info);
    });

    desc_accel_info_.setAccelerationStructures(*accel_);
}

}  // namespace engine