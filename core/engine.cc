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
#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <ranges>

#include "engine.h"
#include "globals.h"

namespace core {

std::filesystem::path getExecutableDir() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    std::filesystem::path exePath(buffer);
    return exePath.parent_path();
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

static std::optional<std::vector<char>> readFile(const std::string& filename) {
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

inline vk::Extent2D chooseSwapExtent(GLFWwindow* window,
    const vk::SurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width
        != std::numeric_limits<uint32_t>::infinity()) {
        return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    return {std::clamp<uint32_t>(width,
                capabilities.minImageExtent.width,
                capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height)};
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

    if (!createSwapChain()) {
        LOG(ERR) << "SwapChain is not created.\n";
        return false;
    }
    LOG(INFO) << "SwapChain created.\n";

    if (!createImageViews()) {
        LOG(ERR) << "ImageViews are not created.\n";
        return false;
    }
    LOG(INFO) << "ImageViews created.\n";

    if (!createGraphicsPipeline()) {
        LOG(ERR) << "GraphicsPipeline is not created.\n";
        return false;
    }
    LOG(INFO) << "GraphicsPipeline created.\n";

    if (!createDynamicState()) {
        LOG(ERR) << "DynamicState is not created.\n";
        return false;
    }
    LOG(INFO) << "DynamicState created.\n";

    LOG(INFO) << "Finished Vulkan initialization.\n";
    return true;
}

[[nodiscard]] vk::raii::ShaderModule MightyEngine::createShaderModule(
    const std::vector<char>& code) const {
    vk::ShaderModuleCreateInfo createInfo{.codeSize =
                                              code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())};
    return logicalDevice_.createShaderModule(createInfo).value;
}

bool MightyEngine::createGraphicsPipeline() {
    auto shaderPath = getExecutableDir() / "shaders" / "slang.spv";
    auto shaderCode = readFile(shaderPath.string());
    if (!shaderCode.has_value()) {
        LOG(ERR) << "Can't find shader file location.\n";
        return false;
    }
    vk::raii::ShaderModule shaderModule =
        createShaderModule(shaderCode.value());

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = shaderModule,
        .pName = "vertMain"};

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = shaderModule,
        .pName = "fragMain"};

    shaderStages_ = {vertShaderStageInfo, fragShaderStageInfo};
    return true;
}

bool MightyEngine::createImageViews() {
    swapChainImages_.clear();

    vk::ImageViewCreateInfo imageViewCreateInfo{.viewType =
                                                    vk::ImageViewType::e2D,
        .format = swapChainSurfaceFormat_.format,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
    for (auto image : swapChainImages_) {
        imageViewCreateInfo.image = image;
        swapChainImageViews_.emplace_back(
            logicalDevice_.createImageView(imageViewCreateInfo).value);
    }
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

bool MightyEngine::createSwapChain() {
    auto surfaceCapabilities =
        physicalDevice_.getSurfaceCapabilitiesKHR(surface_).value;
    swapChainSurfaceFormat_ = chooseSwapSurfaceFormat(
        physicalDevice_.getSurfaceFormatsKHR(surface_).value);
    swapChainExtent_ = chooseSwapExtent(window_, surfaceCapabilities);

    auto minImageCount = surfaceCapabilities.minImageCount < 3
                             ? 3
                             : surfaceCapabilities.minImageCount;
    minImageCount = (surfaceCapabilities.maxImageCount > 0
                        && minImageCount > surfaceCapabilities.maxImageCount)
                        ? surfaceCapabilities.maxImageCount
                        : minImageCount;

    uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0
        && imageCount > surfaceCapabilities.maxImageCount) {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .flags = vk::SwapchainCreateFlagsKHR(),
        .surface = surface_,
        .minImageCount = minImageCount,
        .imageFormat = swapChainSurfaceFormat_.format,
        .imageColorSpace = swapChainSurfaceFormat_.colorSpace,
        .imageExtent = swapChainExtent_,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = chooseSwapPresentMode(
            physicalDevice_.getSurfacePresentModesKHR(surface_).value),
        .clipped = true,
        .oldSwapchain = nullptr};

    uint32_t queueFamilyIndices[] = {graphicsIndex_, presentIndex_};

    if (graphicsIndex_ != presentIndex_) {
        swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;

    } else {
        swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
        swapChainCreateInfo.queueFamilyIndexCount = 0;      // Optional
        swapChainCreateInfo.pQueueFamilyIndices = nullptr;  // Optional
    }
    swapChain_ = logicalDevice_.createSwapchainKHR(swapChainCreateInfo).value;
    swapChainImages_ = swapChain_.getImages().value;

    return true;
}

bool MightyEngine::createDynamicState() {
    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(kDynamicStates.size()),
        .pDynamicStates = kDynamicStates.data()};
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = vk::PrimitiveTopology::eTriangleList};
    vk::Viewport{0.0f,
        0.0f,
        static_cast<float>(swapChainExtent_.width),
        static_cast<float>(swapChainExtent_.height),
        0.0f,
        1.0f};
    vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1,
        .scissorCount = 1};
    vk::PipelineRasterizationStateCreateInfo rasterizer{.depthClampEnable =
                                                            vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = vk::False,
        .depthBiasSlopeFactor = 1.0f,
        .lineWidth = 1.0f};
    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False};
    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
        | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = vk::False;
    colorBlendAttachment.blendEnable = vk::True;
    colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    colorBlendAttachment.dstColorBlendFactor =
        vk::BlendFactor::eOneMinusSrcAlpha;
    colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
    colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
    vk::PipelineColorBlendStateCreateInfo colorBlending{.logicOpEnable =
                                                            vk::False,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment};
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{.setLayoutCount = 0,
        .pushConstantRangeCount = 0};

    pipelineLayout_ =
        logicalDevice_.createPipelineLayout(pipelineLayoutInfo).value;

    return true;
}

bool MightyEngine::createLogicalDevice() {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        physicalDevice_.getQueueFamilyProperties();
    graphicsIndex_ = findQueueFamilies();

    presentIndex_ =
        physicalDevice_.getSurfaceSupportKHR(graphicsIndex_, *surface_).value
            ? graphicsIndex_
            : static_cast<uint32_t>(queueFamilyProperties.size());
    if (presentIndex_ == queueFamilyProperties.size()) {
        for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
            auto hasSupport =
                physicalDevice_.getSurfaceSupportKHR(static_cast<uint32_t>(i),
                    *surface_);
            if ((queueFamilyProperties[i].queueFlags
                    & vk::QueueFlagBits::eGraphics)
                && hasSupport.value) {
                graphicsIndex_ = static_cast<uint32_t>(i);
                presentIndex_ = graphicsIndex_;
                break;
            }
        }
        if (presentIndex_ == queueFamilyProperties.size()) {
            for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
                if (physicalDevice_
                        .getSurfaceSupportKHR(static_cast<uint32_t>(i),
                            *surface_)
                        .value) {
                    presentIndex_ = static_cast<uint32_t>(i);
                    break;
                }
            }
        }
    }
    if ((graphicsIndex_ == queueFamilyProperties.size())
        || (presentIndex_ == queueFamilyProperties.size())) {
        return false;
    }

    float queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{.queueFamilyIndex =
                                                        graphicsIndex_,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority};

    vk::StructureChain<vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        featureChain = {{},
            {.shaderDrawParameters = true},
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
    deviceQueue_ = logicalDevice_.getQueue(graphicsIndex_, 0);
    presentQueue_ = logicalDevice_.getQueue(presentIndex_, 0);

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