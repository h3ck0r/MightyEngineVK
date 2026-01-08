#include "scene.h"

#include "utils/object_utils.h"

namespace engine {
Scene::Scene(const engine::Context& context) : context_(context) {
    LoadMesh();
    CreateBottomLevelAccel();
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

void Scene::CreateBottomLevelAccel() {
    // Create bottom level accel struct
    vk::AccelerationStructureGeometryTrianglesDataKHR triangle_data_;
    triangle_data_.setVertexFormat(vk::Format::eR32G32B32Sfloat);
    triangle_data_.setVertexData(vertex_buffer_->device_address());
    triangle_data_.setVertexStride(sizeof(Vertex));
    triangle_data_.setMaxVertex(static_cast<uint32_t>(vertices_.size()));
    triangle_data_.setIndexType(vk::IndexType::eUint32);
    triangle_data_.setIndexData(index_buffer_->device_address());

    vk::AccelerationStructureGeometryKHR triangle_geometry_;
    triangle_geometry_.setGeometryType(vk::GeometryTypeKHR::eTriangles);
    triangle_geometry_.setGeometry({triangle_data_});
    triangle_geometry_.setFlags(vk::GeometryFlagBitsKHR::eOpaque);

    uint32_t primitive_count_ = static_cast<uint32_t>(indices_.size() / 3);

    bottom_accel_ = std::make_unique<Accel>(context_,
        triangle_geometry_,
        primitive_count_,
        vk::AccelerationStructureTypeKHR::eBottomLevel);
}
}  // namespace engine