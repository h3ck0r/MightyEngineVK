#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

#include <iostream>

#define MTY_CHECK(result)                                                      \
    do {                                                                       \
        mty::handleVulkanResult(result, __FILE__, __LINE__);                   \
    } while (0)

namespace mty {

inline void handleVulkanResult(VkResult result, const char* file, int line) {
    switch (result) {
        case VK_SUCCESS:
            return;
        default:
            std::cerr << "[" << string_VkResult(result) << "][" << file << ":"
                      << line << "]\n";
    }
}

inline void windowErrorCallback(int error, const char* description) {
    std::cerr << description << "\n";
}

inline VkBool32 debugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    std::string messageSeverityToString = "";
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            messageSeverityToString = "V";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            messageSeverityToString = "I";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            messageSeverityToString = "W";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            messageSeverityToString = "E";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
            break;
    }
    std::string messageTypeToString = "";
    switch (messageTypes) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
            messageTypeToString = "General";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            messageTypeToString = "Validation";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            messageTypeToString = "Performance";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
            messageTypeToString = "Device Address";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT:
            break;
    }

    std::cout << "[" << messageSeverityToString << "]" << "["
              << messageTypeToString << "] " << pCallbackData->pMessage << "\n";

    return VK_FALSE;
}
}  // namespace mty