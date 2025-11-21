#include "context.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <iterator>
#include <vector>

namespace context {

void MightyContext::run() {
    createInstance();
    createDevice();

    vkDestroyInstance(instance, nullptr);
}

void MightyContext::createDevice() {
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());
    physicalDevice = physicalDevices.front();

    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
    auto major = VK_API_VERSION_MAJOR(physicalDeviceProperties.apiVersion);
    auto minor = VK_API_VERSION_MINOR(physicalDeviceProperties.apiVersion);
    auto patch = VK_API_VERSION_PATCH(physicalDeviceProperties.apiVersion);
    std::cout << "Supported VK Version by DEVICE:   " << major << "." << minor
              << "." << patch << "\n";
}

void MightyContext::createInstance() {
    VkApplicationInfo appInfo;
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.applicationVersion = 1.0;
    appInfo.apiVersion = VK_API_VERSION_1_4;
    appInfo.pApplicationName = "Mighty Application";
    appInfo.pEngineName = "Mighty Engine";

    VkValidationFeaturesEXT validationExtensionsInfo;
    validationExtensionsInfo.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    validationExtensionsInfo.pNext = nullptr;
    validationExtensionsInfo.pEnabledValidationFeatures = VALIDATION_EXTENSIONS;
    validationExtensionsInfo.enabledValidationFeatureCount =
        std::size(VALIDATION_EXTENSIONS);

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    debugCreateInfo.pNext = &validationExtensionsInfo;
    debugCreateInfo.sType =
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity =
        VkDebugUtilsMessageSeverityFlagBitsEXT::
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
        | VkDebugUtilsMessageSeverityFlagBitsEXT::
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
        | VkDebugUtilsMessageSeverityFlagBitsEXT::
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VkDebugUtilsMessageSeverityFlagBitsEXT::
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    debugCreateInfo.messageType =
        VkDebugUtilsMessageTypeFlagBitsEXT::
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
        | VkDebugUtilsMessageTypeFlagBitsEXT::
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugMessengerCallback;

    VkInstanceCreateInfo createInfo;
    createInfo.pNext = &debugCreateInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = std::size(INSTANCE_LAYERS);
    createInfo.ppEnabledLayerNames = INSTANCE_LAYERS;
    createInfo.enabledExtensionCount = std::size(INSTANCE_EXTENSIONS);
    createInfo.ppEnabledExtensionNames = INSTANCE_EXTENSIONS;
    createInfo.flags = 0;

    vkCreateInstance(&createInfo, nullptr, &instance);
    uint32_t version;
    vkEnumerateInstanceVersion(&version);
    auto major = VK_API_VERSION_MAJOR(version);
    auto minor = VK_API_VERSION_MINOR(version);
    auto patch = VK_API_VERSION_PATCH(version);
    std::cout << "Supported VK Version by INSTANCE: " << major << "." << minor
              << "." << patch << "\n";
}

}  // namespace context