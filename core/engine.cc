#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_EXCEPTIONS
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <GLFW/glfw3.h>

#include <algorithm>
#include <iostream>
#include <ranges>

#include "engine.h"
#include "globals.h"

namespace core {

static VKAPI_ATTR vk::Bool32 VKAPI_CALL
    debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*) {
    LOG(ERROR) << "[" << to_string(type) << "]"
               << " " << pCallbackData->pMessage << "\n";

    return vk::False;
}

Engine::Engine() {}

void Engine::run() {
    initWindow();
    initVK();
    loop();
    cleanup();
}
bool Engine::initWindow() {
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
void Engine::loop() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
    }
}
bool Engine::initVK() {
    LOG(INFO) << "Initilazing " << kAppName << "\n";
    LOG(INFO) << "Enable Validation Layers: " << kEnableValidationLayers << "\n"
              << "More debugging:" << kMoreDebugging << "\n";
    if (!createVKInstance()) {
        LOG(ERROR) << "Vulkan Instance is not initialized";
    }
    LOG(INFO) << "VK Instance created.\n";
    if (!setupDebugMessenger()) {
        LOG(ERROR) << "Debug Messenger is not initialized";
    }
    LOG(INFO) << "Debug Messenger initialized.\n";
    if (!pickPhysicalDevice()) {
        LOG(ERROR) << "Physical device is not picked";
    }
    LOG(INFO) << "Physical device picked.\n";
    if (!createLogicalDevice()) {
        LOG(ERROR) << "Logical Device is not created";
    }
    LOG(INFO) << "Logical Device created.\n";
    LOG(INFO) << "Finished Vulkan initialization.\n";
    return true;
}

bool Engine::createLogicalDevice() {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        physicalDevice_.getQueueFamilyProperties();
    auto graphicsIndex = findQueueFamilies();
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

    // TODO: add result checks
    auto [result, logicalDevice] =
        physicalDevice_.createDevice(deviceCreateInfo);
    if (result != vk::Result::eSuccess) {
        return false;
    }
    logicalDevice_ = std::move(logicalDevice);
    deviceQueue_ = logicalDevice_.getQueue(graphicsIndex, 0);

    return true;
}

bool Engine::setupDebugMessenger() {
    if (!kEnableValidationLayers) {
        return false;
    }
    LOG(INFO) << "1";

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
    LOG(INFO) << "2";

    // TODO: add result checks
    auto [result, debugMessenger] = instance_.createDebugUtilsMessengerEXT(
        debugUtilsMessengerCreateInfoEXT);

    LOG(INFO) << "2.5";
    debugMessenger_ = std::move(debugMessenger);
    LOG(INFO) << "3";
    if (result != vk::Result::eSuccess) {
        return false;
    }
    LOG(INFO) << "4";
    return true;
}

bool Engine::pickPhysicalDevice() {
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    // TODO: add result checks
    auto [result, physicalDevices] = instance_.enumeratePhysicalDevices();
    if (result != vk::Result::eSuccess) {
        return false;
    }
    if (physicalDevices.empty()) {
        LOG(ERROR) << "No Physical devices" << "\n";
        return false;
    }
    if (kMoreDebugging) {
        LOG(INFO) << "Available Devices:\n";
        for (const auto& device : physicalDevices) {
            LOG(INFO) << device.getProperties().deviceName << "\n";
        }
    }
    physicalDevice_ = std::move(physicalDevices[0]);
    return true;
}

uint32_t Engine::findQueueFamilies() {
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

bool Engine::createVKInstance() {
    constexpr vk::ApplicationInfo appInfo{.pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion14};
    // VALIDATION LAYERS
    std::vector<char const*> requiredLayers;
    if (kEnableValidationLayers) {
        requiredLayers.assign(kValidationLayers.begin(),
            kValidationLayers.end());
        // TODO: add result checks
        auto [result, availableLayers] =
            context_.enumerateInstanceLayerProperties();
        if (result != vk::Result::eSuccess) {
            return false;
        }
        if (kMoreDebugging) {
            LOG(INFO) << "Available layers:\n";
            for (const auto& layer : availableLayers) {
                LOG(INFO) << layer.layerName << "\n";
            }
        }
        for (uint32_t i = 0; i < kValidationLayers.size(); ++i) {
            bool found = false;
            for (const auto& layerProperty : availableLayers) {
                if (std::string_view(layerProperty.layerName)
                    == requiredLayers[i]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                LOG(ERROR) << "Required Validation Layer not supported: "
                           << std::string(requiredLayers[i]);
                return false;
            }
        }
    }
    // END OF VALIDATION LAYERS
    // DEVICE EXTENSIONS
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions =
        glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    // TODO: add result check
    auto [result, available_extensions] =
        context_.enumerateInstanceExtensionProperties();
    if (result != vk::Result::eSuccess) {
        return false;
    }
    if (kMoreDebugging) {
        LOG(INFO) << "Available extensions:\n";
        for (const auto& extension : available_extensions) {
            LOG(INFO) << extension.extensionName << "\n";
        }
    }
    for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
        bool found = false;
        for (const auto& extensionProperty : available_extensions) {
            if (std::string_view(extensionProperty.extensionName)
                == glfwExtensions[i]) {
                found = true;
                break;
            }
        }

        if (!found) {
            LOG(ERROR) << "Required GLFW extension not supported: "
                       << std::string(glfwExtensions[i]);
            return false;
        }
    }
    // END OF DEVICE EXTENSIONS
    // CREATE INSTANCE
    vk::InstanceCreateInfo createInfo{.pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(glfwExtensionCount),
        .ppEnabledExtensionNames = glfwExtensions};

    // TODO: add result checks
    auto [instance_result, instance] = context_.createInstance(createInfo);
    if (instance_result != vk::Result::eSuccess) {
        return false;
    }
    instance_ = std::move(instance);
    // END OF CREATE INSTANCE
    return true;
}

void Engine::cleanup() {
    glfwDestroyWindow(window_);
    glfwTerminate();
}

}  // namespace core