#include "scene.h"

#include "utils/object_utils.h"

namespace engine {
Scene::Scene(const engine::Context& context) : context_(context) {
    LoadMesh();
}

void Scene::LoadMesh() {
    // Load mesh
    LoadFromFile(vertices_, indices_, faces_);

    vertex_buffer_ = std::make_unique<Buffer>(context_.device(),
        context_.physical_device(),
        Buffer::Type::AccelInput,
        sizeof(Vertex) * vertices_.size(),
        vertices_.data());
    index_buffer_ = std::make_unique<Buffer>(context_.device(),
        context_.physical_device(),
        Buffer::Type::AccelInput,
        sizeof(uint32_t) * indices_.size(),
        indices_.data());
    face_buffer_ = std::make_unique<Buffer>(context_.device(),
        context_.physical_device(),
        Buffer::Type::AccelInput,
        sizeof(Face) * faces_.size(),
        faces_.data());
}
}  // namespace engine