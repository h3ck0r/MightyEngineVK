#include "context.h"

#include <vulkan/vulkan_core.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <vector>

#include "utils.h"

namespace mty {

void MtyContext::run() {
    createInstance();
    createDeviceAndQueue();
    createSurfaceAndSwapchain();
    loop();
    cleanup();
}

void MtyContext::loop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void MtyContext::createSurfaceAndSwapchain() {
    glfwInit();
    glfwSetErrorCallback(windowErrorCallback);
    window = glfwCreateWindow(WINDOW_WIDTH,
        WINDOW_HEIGHT,
        "Mighty Engine",
        nullptr,
        nullptr);
    VkWin32SurfaceCreateInfoKHR win32CreateSurfaceInfo{
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hwnd = glfwGetWin32Window(window)};
    MTY_CHECK(vkCreateWin32SurfaceKHR(instance,
        &win32CreateSurfaceInfo,
        nullptr,
        &surface));
}

void MtyContext::createDeviceAndQueue() {
    uint32_t deviceCount;
    MTY_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    MTY_CHECK(vkEnumeratePhysicalDevices(instance,
        &deviceCount,
        physicalDevices.data()));

    for (size_t i = 0; i < deviceCount; i++) {
        VkPhysicalDeviceProperties2 physicalDeviceProperties{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = nullptr,
        };

        vkGetPhysicalDeviceProperties2(physicalDevices[i],
            &physicalDeviceProperties);
        auto properties = physicalDeviceProperties.properties;

        std::cout << properties.deviceName << "\n";
        std::cout << "  Device Type: " << properties.deviceType << "\n";
        auto major = VK_API_VERSION_MAJOR(properties.apiVersion);
        auto minor = VK_API_VERSION_MINOR(properties.apiVersion);
        auto patch = VK_API_VERSION_PATCH(properties.apiVersion);
        std::cout << "  Supported VK Version by DEVICE: " << major << "."
                  << minor << "." << patch << "\n";
        std::cout << "  Vendor ID: " << properties.vendorID << "\n";
        std::cout << "  Device ID: " << properties.deviceID << "\n";
    }

    physicalDevice = physicalDevices.front();

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice,
        &queueFamilyCount,
        nullptr);
    std::vector<VkQueueFamilyProperties2> queueFamilyProperties(
        queueFamilyCount);
    for (auto& properties : queueFamilyProperties) {
        properties.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
    }
    vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice,
        &queueFamilyCount,
        queueFamilyProperties.data());
    uint32_t selectedQueueFamilyIndex = 0;
    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if (queueFamilyProperties[i].queueFamilyProperties.queueFlags
                & VK_QUEUE_GRAPHICS_BIT
            && queueFamilyProperties[i].queueFamilyProperties.queueFlags
                   & VK_QUEUE_COMPUTE_BIT) {
            selectedQueueFamilyIndex = i;
            break;
        }
    }
    const float queuePriorities[] = {1.0};

    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = selectedQueueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = queuePriorities,
    };

    VkPhysicalDevice8BitStorageFeatures device8BitStorageFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES,
        .storageBuffer8BitAccess = VK_TRUE};

    VkPhysicalDeviceScalarBlockLayoutFeatures deviceScalarBlockLayoutFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES,
        .pNext = &device8BitStorageFeatures,
        .scalarBlockLayout = VK_TRUE};

    VkPhysicalDeviceVulkanMemoryModelFeatures deviceVulkanMemoryModelFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES,
        .pNext = &deviceScalarBlockLayoutFeatures,
        .vulkanMemoryModel = VK_TRUE,
        .vulkanMemoryModelDeviceScope = VK_TRUE};

    VkPhysicalDeviceBufferDeviceAddressFeatures
        deviceBufferDeviceAddressFeatures{
            .sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
            .pNext = &deviceVulkanMemoryModelFeatures,
            .bufferDeviceAddress = VK_TRUE,
        };

    VkPhysicalDeviceTimelineSemaphoreFeatures deviceTimelineSemaphoreFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
        .pNext = &deviceBufferDeviceAddressFeatures,
        .timelineSemaphore = VK_TRUE,
    };

    VkPhysicalDeviceFeatures2 physicalDeviceFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &deviceTimelineSemaphoreFeatures,
        .features = {
            .vertexPipelineStoresAndAtomics = VK_TRUE,
            .fragmentStoresAndAtomics = VK_TRUE,
            .shaderInt64 = VK_TRUE,
        }};

    VkDeviceCreateInfo deviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &physicalDeviceFeatures,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = std::size(DEVICE_EXTENSIONS),
        .ppEnabledExtensionNames = DEVICE_EXTENSIONS};

    MTY_CHECK(
        vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));

    VkDeviceQueueInfo2 deviceQueueInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
        .queueFamilyIndex = selectedQueueFamilyIndex,
        .queueIndex = 0};
    vkGetDeviceQueue2(device, &deviceQueueInfo, &queue);
}

void MtyContext::createInstance() {
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Mighty Application",
        .applicationVersion = 1,
        .pEngineName = "Mighty Engine",
        .engineVersion = 1,
        .apiVersion = VK_API_VERSION_1_4,
    };

    VkValidationFeaturesEXT validationExtensionsInfo = {
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .enabledValidationFeatureCount = std::size(VALIDATION_EXTENSIONS),
        .pEnabledValidationFeatures = VALIDATION_EXTENSIONS,
    };

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = &validationExtensionsInfo,
        .messageSeverity = VkDebugUtilsMessageSeverityFlagBitsEXT::
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
                           | VkDebugUtilsMessageSeverityFlagBitsEXT::
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                           | VkDebugUtilsMessageSeverityFlagBitsEXT::
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                           | VkDebugUtilsMessageSeverityFlagBitsEXT::
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
        .messageType = VkDebugUtilsMessageTypeFlagBitsEXT::
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
                       | VkDebugUtilsMessageTypeFlagBitsEXT::
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
        .pfnUserCallback = debugMessengerCallback,
    };

    VkInstanceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &debugCreateInfo,
        .flags = 0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = std::size(INSTANCE_LAYERS),
        .ppEnabledLayerNames = INSTANCE_LAYERS,
        .enabledExtensionCount = std::size(INSTANCE_EXTENSIONS),
        .ppEnabledExtensionNames = INSTANCE_EXTENSIONS,
    };

    MTY_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));

    uint32_t version;
    MTY_CHECK(vkEnumerateInstanceVersion(&version));
    auto major = VK_API_VERSION_MAJOR(version);
    auto minor = VK_API_VERSION_MINOR(version);
    auto patch = VK_API_VERSION_PATCH(version);
    std::cout << "Supported VK Version by INSTANCE: " << major << "." << minor
              << "." << patch << "\n";
}

void MtyContext::cleanup() {
    vkDestroySurfaceKHR(instance, surface, nullptr);
    glfwTerminate();
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
}

}  // namespace mty