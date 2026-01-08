#include "context.h"
#include "utils/accel.h"
#include "utils/buffer.h"
#include "utils/primitives.h"

namespace engine {

class Scene {
   public:
    Scene(const engine::Context& context);
    // No copy
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    const Buffer& vertex_buffer() const { return *vertex_buffer_.get(); }
    const Buffer& index_buffer() const { return *index_buffer_.get(); }
    const Buffer& face_buffer() const { return *face_buffer_.get(); }

    const std::vector<Vertex>& vertices() const { return vertices_; }
    const std::vector<uint32_t>& indices() const { return indices_; }
    const std::vector<Face>& faces() const { return faces_; }
    const Accel& bottom_accel() const { return *bottom_accel_.get(); }

   private:
    void LoadMesh();
    void CreateBottomLevelAccel();
    const engine::Context& context_;
    std::unique_ptr<Buffer> vertex_buffer_;
    std::unique_ptr<Buffer> index_buffer_;
    std::unique_ptr<Buffer> face_buffer_;
    std::unique_ptr<Accel> bottom_accel_;

    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<Face> faces_;
};

}  // namespace engine