#ifndef UITLS_
#define UITLS_

#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_EXCEPTIONS

#include <cstdint>

#define TINYOBJLOADER_IMPLEMENTATION
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <windows.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "tiny_obj_loader.h"
#include "vulkan/vulkan.hpp"

namespace core {

using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;

#define LOG(severity)          LogStream(severity, __FILE__, __LINE__)
#define LOG_VK(severity, type) LogStream(severity, type, __FILE__, __LINE__)
enum LogType { INFO, ERR, WARR };

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
};

struct Material {
    glm::vec3 diffuse;
    glm::vec3 emission;
};

inline uint32_t findMemoryType(const vk::PhysicalDevice& physicalDevice,
    uint32_t typeFilter,
    vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties =
        physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i))
            && (memProperties.memoryTypes[i].propertyFlags & properties)
                   == properties) {
            return i;
        }
    }
    return -1;
}

inline void loadFromFile(std::vector<Vertex>& vertices,
    std::vector<uint32_t>& indices,
    std::vector<Material>& materials) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> _materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib,
            &shapes,
            &_materials,
            &warn,
            &err,
            "../assets/CornellBox-Original.obj",
            "../assets")) {
        throw std::runtime_error(warn + err);
    }

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};
            vertex.pos[0] = attrib.vertices[3 * index.vertex_index + 0];
            vertex.pos[1] = -attrib.vertices[3 * index.vertex_index + 1];
            vertex.pos[2] = attrib.vertices[3 * index.vertex_index + 2];
            vertices.push_back(vertex);
            indices.push_back(static_cast<uint32_t>(indices.size()));
        }
        for (const auto& matIndex : shape.mesh.material_ids) {
            Material mat;
            mat.diffuse[0] = materials[matIndex].diffuse[0];
            mat.diffuse[1] = materials[matIndex].diffuse[1];
            mat.diffuse[2] = materials[matIndex].diffuse[2];
            mat.emission[0] = materials[matIndex].emission[0];
            mat.emission[1] = materials[matIndex].emission[1];
            mat.emission[2] = materials[matIndex].emission[2];
            materials.push_back(mat);
        }
    }
}
inline vk::VertexInputBindingDescription getBindingDescription() {
    return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
}

inline std::array<vk::VertexInputAttributeDescription, 2>
    getAttributeDescriptor() {
    return {vk::VertexInputAttributeDescription(0,
                0,
                vk::Format::eR32G32Sfloat,
                offsetof(Vertex, pos)),

        vk::VertexInputAttributeDescription(1,
            0,
            vk::Format::eR32G32B32Sfloat,
            offsetof(Vertex, color))};
}

inline const char* VkLogSeverityToString(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity) {
    switch (severity) {
        case eVerbose:
            return "V";
        case eWarning:
            return "W";
        case eError:
            return "E";
        default:
            return "UNKNOWN";
    }
}

inline const char* LogSeverityToString(LogType type) {
    switch (type) {
        case INFO:
            return "INFO";
        case ERR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

inline const char* LogTypeToString(vk::DebugUtilsMessageTypeFlagsEXT type) {
    if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)
        return "General";
    if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
        return "Validation";
    if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
        return "Performance";
    if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding)
        return "DeviceAddressBinding";
    return "UNKNOWN";
}

class LogStream {
   public:
    LogStream(LogType severity, const char* file, int line)
        : severity_(severity), file_(file), line_(line) {}

    LogStream(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const char* file,
        int line)
        : vkSeverity_(severity), type_(type), file_(file), line_(line) {}

    ~LogStream() {
        std::ostream& out =
            (severity_ == ERR || vkSeverity_ == eError) ? std::cerr : std::cout;
        std::filesystem::path p(file_);
        if (type_ != vk::DebugUtilsMessageTypeFlagsEXT{}) {
            out << "[" << VkLogSeverityToString(vkSeverity_) << "]" << "["
                << LogTypeToString(type_) << "]" << stream_.str();
        } else {
            out << "[" << LogSeverityToString(severity_) << "]"
                << "[" << p.filename().string() << ":" << line_ << "] "
                << stream_.str();
        }
        out.flush();
    }

    template <typename T>
    LogStream& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

   private:
    vk::DebugUtilsMessageSeverityFlagBitsEXT vkSeverity_;
    LogType severity_;
    vk::DebugUtilsMessageTypeFlagsEXT type_;
    const char* file_;
    int line_;
    std::ostringstream stream_;
};

inline void setWindowIcon(GLFWwindow* window, const char* iconPath) {
    int width, height, channels;
    unsigned char* pixels =
        stbi_load(iconPath, &width, &height, &channels, 4);  // force RGBA
    if (!pixels) {
        fprintf(stderr, "Failed to load icon: %s\n", iconPath);
        return;
    }

    GLFWimage images[1];
    images[0].width = width;
    images[0].height = height;
    images[0].pixels = pixels;

    glfwSetWindowIcon(window, 1, images);

    stbi_image_free(pixels);
}

inline std::filesystem::path getExecutableDir() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    std::filesystem::path exePath(buffer);
    return exePath.parent_path();
}

inline VKAPI_ATTR vk::Bool32 VKAPI_CALL
    debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*) {
    LOG_VK(severity, type) << pCallbackData->pMessage << "\n";
    return vk::False;
}

inline std::optional<std::vector<char>> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        return std::nullopt;
    }
    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();

    return buffer;
}

inline vk::PresentModeKHR chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

inline vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb
            && availableFormat.colorSpace
                   == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

}  // namespace core

#endif  // UTILS_