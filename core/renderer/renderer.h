#ifndef VK_RENDERER_H_
#define VK_RENDERER_H_

#include <memory>

#include "../vulkan/command_buffer.h"
#include "utils/accel.h"
#include "utils/buffer.h"
#include "utils/image.h"
#include "vulkan/context.h"

namespace engine {

struct Vertex {
    float position[3];
};

struct Face {
    float diffuse[3];
    float emission[3];
};

class Renderer {
   public:
    Renderer();
    // No copy
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void MainLoop();

   private:
    engine_init::Context context_;
    std::unique_ptr<engine_init::CommandBuffer> command_buffer_;
    std::unique_ptr<Image> output_image_;
    std::unique_ptr<Buffer> vertex_buffer_;
    std::unique_ptr<Buffer> index_buffer_;
    std::unique_ptr<Buffer> face_buffer_;
    std::unique_ptr<Accel> bottom_accel_;
    std::unique_ptr<Accel> top_accel_;
    vk::UniquePipeline pipeline_;
    vk::UniquePipelineLayout pipeline_layout_;
    vk::UniqueDescriptorSet desc_set_;
    vk::StridedDeviceAddressRegionKHR raygen_region_;
    vk::StridedDeviceAddressRegionKHR miss_region_;
    vk::StridedDeviceAddressRegionKHR hit_region_;

    std::unique_ptr<Buffer> raygen_SBT_;
    std::unique_ptr<Buffer> miss_SBT_;
    std::unique_ptr<Buffer> hit_SBT_;
};
}  // namespace engine
#endif