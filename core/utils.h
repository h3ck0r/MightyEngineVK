#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_EXCEPTIONS

#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <numeric>
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "globals.h"

namespace core {

using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;

#define LOG(severity)          LogStream(severity, __FILE__, __LINE__)
#define LOG_VK(severity, type) LogStream(severity, type, __FILE__, __LINE__)
enum LogType { INFO, ERR, WARR };

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
                << LogTypeToString(type_) << "]"
                << stream_.str();
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

inline std::vector<const char*> getInstanceExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions =
        glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions,
        glfwExtensions + glfwExtensionCount);

    if (kEnableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

}  // namespace core