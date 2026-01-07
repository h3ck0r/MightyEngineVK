#ifndef VK_OBJECT_UTILS_H_
#define VK_OBJECT_UTILS_H_

#include <tiny_obj_loader.h>

#include <vulkan/vulkan.hpp>

#include "primitives.h"

namespace engine {

void LoadFromFile(std::vector<Vertex>& vertices,
    std::vector<uint32_t>& indices,
    std::vector<Face>& faces);

std::vector<char> ReadFile(const std::string& filename);

uint32_t FindMemoryType(uint32_t type_filter,
    vk::MemoryPropertyFlags properties,
    vk::PhysicalDeviceMemoryProperties mem_properties);

}  // namespace engine

#endif