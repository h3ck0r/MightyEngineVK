#ifndef CONTEXT_
#define CONTEXT_

#include <vulkan/vulkan_core.h>

#include <iostream>
#include <string>

namespace context {
inline constexpr const char* INSTANCE_EXTENSIONS[] = {
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
inline constexpr const char* INSTANCE_LAYERS[] = {
    "VK_LAYER_KHRONOS_validation"};
inline constexpr const VkValidationFeatureEnableEXT VALIDATION_EXTENSIONS[] = {
    VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
    VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
    VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
};

struct MightyContext {
    void run();
    void createInstance();
    void createDevice();

    VkInstance instance;

    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties physicalDeviceProperties;
};
}  // namespace context

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

#endif  // CONTEXT_