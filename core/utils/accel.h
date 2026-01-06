
#ifndef VK_ACCEL_H_
#define VK_ACCEL_H_

#include <vulkan/vulkan.hpp>

#include "../vulkan/context.h"
#include "buffer.h"

namespace engine {
struct Accel {
    Accel(const engine_init::Context& context,
        vk::AccelerationStructureGeometryKHR geometry,
        uint32_t primitive_count,
        vk::AccelerationStructureTypeKHR type);
    // No copy
    Accel(const Accel&) = delete;
    Accel& operator=(const Accel&) = delete;

    std::unique_ptr<Buffer> buffer_;
    vk::UniqueAccelerationStructureKHR accel_;
    vk::WriteDescriptorSetAccelerationStructureKHR desc_accel_info_;
};
}  // namespace engine

#endif