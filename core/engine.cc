#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_EXCEPTIONS
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <algorithm>
#include <iostream>
#include <ranges>

#include "engine.h"
#include "globals.h"

namespace core {

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

static VKAPI_ATTR vk::Bool32 VKAPI_CALL
    debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*) {
    LOG(ERR) << "[" << to_string(type) << "]"
             << " " << pCallbackData->pMessage << "\n";
    return vk::False;
}

MightyEngine::MightyEngine() {}

void MightyEngine::run() {
    initWindow();
    initVK();
    loop();
    cleanup();
}
bool MightyEngine::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(kWindowWidth,
        kWindowHeight,
        kAppName.data(),
        nullptr,
        nullptr);
    return true;
}
void MightyEngine::loop() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
    }
}
bool MightyEngine::initVK() {
    LOG(INFO) << "Initilazing " << kAppName << "\n";
    LOG(INFO) << "Enable Validation Layers: " << kEnableValidationLayers << "\n"
              << "More debugging:" << kMoreLogs << "\n";
    if (!createVKInstance()) {
        LOG(ERR) << "Vulkan Instance is not initialized.\n";
        return false;
    }
    LOG(INFO) << "VK Instance created.\n";

    if (!setupDebugMessenger()) {
        LOG(ERR) << "Debug Messenger is not initialized.\n";
        return false;
    }
    LOG(INFO) << "Debug Messenger initialized.\n";

    if (!createSurface()) {
        LOG(ERR) << "Surface is not initialized.\n";
    }
    LOG(INFO) << "Surface is initialized\n";

    if (!pickPhysicalDevice()) {
        LOG(ERR) << "Physical device is not picked.\n";
        return false;
    }
    LOG(INFO) << "Physical device picked.\n";

    if (!createLogicalDevice()) {
        LOG(ERR) << "Logical Device is not created.\n";
        return false;
    }
    LOG(INFO) << "Logical Device created.\n";

    LOG(INFO) << "Finished Vulkan initialization.\n";
    return true;
}

bool MightyEngine::createSurface() {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*instance_, window_, nullptr, &surface) != 0) {
        return false;
    }
    surface_ = vk::raii::SurfaceKHR(instance_, surface);

    return true;
}

bool MightyEngine::createLogicalDevice() {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        physicalDevice_.getQueueFamilyProperties();
    auto graphicsIndex = findQueueFamilies();

    auto presentIndex =
        physicalDevice_.getSurfaceSupportKHR(graphicsIndex, *surface_).value
            ? graphicsIndex
            : static_cast<uint32_t>(queueFamilyProperties.size());
    if (presentIndex == queueFamilyProperties.size()) {
        for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
            auto hasSupport =
                physicalDevice_.getSurfaceSupportKHR(static_cast<uint32_t>(i),
                    *surface_);
            if ((queueFamilyProperties[i].queueFlags
                    & vk::QueueFlagBits::eGraphics)
                && hasSupport.value) {
                graphicsIndex = static_cast<uint32_t>(i);
                presentIndex = graphicsIndex;
                break;
            }
        }
        if (presentIndex == queueFamilyProperties.size()) {
            for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
                if (physicalDevice_
                        .getSurfaceSupportKHR(static_cast<uint32_t>(i),
                            *surface_)
                        .value) {
                    presentIndex = static_cast<uint32_t>(i);
                    break;
                }
            }
        }
    }
    if ((graphicsIndex == queueFamilyProperties.size())
        || (presentIndex == queueFamilyProperties.size())) {
        return false;
    }

    float queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{.queueFamilyIndex =
                                                        graphicsIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority};

    vk::StructureChain<vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        featureChain = {{},
            {.dynamicRendering = true},
            {.extendedDynamicState = true}};
    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount =
            static_cast<uint32_t>(kDeviceExtensions.size()),
        .ppEnabledExtensionNames = kDeviceExtensions.data()};

    auto [result, logicalDevice] =
        physicalDevice_.createDevice(deviceCreateInfo);
    if (result != vk::Result::eSuccess) {
        return false;
    }
    logicalDevice_ = std::move(logicalDevice);
    deviceQueue_ = logicalDevice_.getQueue(graphicsIndex, 0);
    presentQueue_ = logicalDevice_.getQueue(presentIndex, 0);

    return true;
}

bool MightyEngine::setupDebugMessenger() {
    if (!kEnableValidationLayers) {
        return false;
    }

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
        | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
        | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
        .messageSeverity = severityFlags,
        .messageType = messageTypeFlags,
        .pfnUserCallback = &debugCallback};

    // TODO: add result checks
    auto [result, debugMessenger] = instance_.createDebugUtilsMessengerEXT(
        debugUtilsMessengerCreateInfoEXT);

    debugMessenger_ = std::move(debugMessenger);
    if (result != vk::Result::eSuccess) {
        return false;
    }
    return true;
}

bool MightyEngine::pickPhysicalDevice() {
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    // TODO: add result checks
    auto [result, physicalDevices] = instance_.enumeratePhysicalDevices();
    if (result != vk::Result::eSuccess) {
        return false;
    }
    if (physicalDevices.empty()) {
        LOG(ERR) << "No Physical devices" << "\n";
        return false;
    }
    if (kMoreLogs) {
        LOG(INFO) << "Available Devices:\n";
        for (const auto& device : physicalDevices) {
            LOG(INFO) << device.getProperties().deviceName << "\n";
        }
    }
    physicalDevice_ = std::move(physicalDevices[0]);
    return true;
}

uint32_t MightyEngine::findQueueFamilies() {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        physicalDevice_.getQueueFamilyProperties();

    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;

    for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i) {
        if (queueFamilyProperties[i].queueFlags
            & vk::QueueFlagBits::eGraphics) {
            graphicsQueueFamilyIndex = i;
            break;
        }
    }

    return graphicsQueueFamilyIndex;
}

bool MightyEngine::createVKInstance() {
    constexpr vk::ApplicationInfo appInfo{.pApplicationName = kAppName.data(),
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = kAppName.data(),
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion14};
    std::vector<char const*> requiredLayers;
    if (kEnableValidationLayers) {
        requiredLayers.assign(kValidationLayers.begin(),
            kValidationLayers.end());
        auto [result, availableLayers] =
            context_.enumerateInstanceLayerProperties();
        if (result != vk::Result::eSuccess) {
            return false;
        }
    }

    auto extensions = getInstanceExtensions();

    vk::InstanceCreateInfo createInfo{.pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()};

    auto [instance_result, instance] = context_.createInstance(createInfo);
    if (instance_result != vk::Result::eSuccess) {
        return false;
    }
    instance_ = std::move(instance);
    // END OF CREATE INSTANCE
    return true;
}

void MightyEngine::cleanup() {
    glfwDestroyWindow(window_);
    glfwTerminate();
}

}  // namespace core