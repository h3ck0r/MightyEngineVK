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

void MightyEngine::recordCommandBuffer(uint32_t imageIndex) {
    // Before starting rendering, transition the swapchain image to
    // COLOR_ATTACHMENT_OPTIMAL
    transitioImageLayout(imageIndex,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},  // srcAccessMask (no need to wait for previous operations)
        vk::AccessFlagBits2::eColorAttachmentWrite,          // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,  // srcStage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput   // dstStage
    );

    vk::ClearColorValue clearColor(
        std::array<float, 4>({0.0f, 0.0f, 0.0f, 1.0f}));
    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView = swapChainImageViews_[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = {clearColor}};

    vk::RenderingInfo renderingInfo = {.renderArea = {.offset = {0, 0},
                                           .extent = swapChainExtent_},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo};

    commandBuffer_.beginRendering(renderingInfo);
    commandBuffer_.bindPipeline(vk::PipelineBindPoint::eGraphics,
        graphicsPipeline_);
    commandBuffer_.setViewport(0,
        vk::Viewport(0.0f,
            0.0f,
            static_cast<float>(swapChainExtent_.width),
            static_cast<float>(swapChainExtent_.height),
            0.0f,
            1.0f));
    commandBuffer_.setScissor(0,
        vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent_));
    commandBuffer_.draw(3, 1, 0, 0);
    commandBuffer_.endRendering();

    transitioImageLayout(imageIndex,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,          // srcAccessMask
        {},                                                  // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,  // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe            // dstStage
    );
    commandBuffer_.end();
}

void MightyEngine::transitioImageLayout(uint32_t imageIndex,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::AccessFlags2 srcAccessMask,
    vk::AccessFlags2 dstAccessMask,
    vk::PipelineStageFlags2 srcStageMask,
    vk::PipelineStageFlags2 dstStageMask) {
    vk::ImageMemoryBarrier2 barrier = {.srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapChainImages_[imageIndex],
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1}};
    vk::DependencyInfo dependencyInfo = {.dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier};
    commandBuffer_.pipelineBarrier2(dependencyInfo);
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
        drawFrame();
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
        LOG(ERR) << "Graphics Pipeline is not created.\n";
        return false;
    }
    LOG(INFO) << "Graphics Pipeline created.\n";

    if (!createCommandPool()) {
        LOG(ERR) << "Command Pool is not created.\n";
        return false;
    }

    if (!createCommandBuffer()) {
        LOG(ERR) << "Command Buffer is not created.\n";
        return false;
    }
    LOG(INFO) << "Command Buffer created.\n";

    if (!createSyncObjects()) {
        LOG(ERR) << "Sync Objects are not created.\n";
        return false;
    }
    LOG(INFO) << "Sync Objects created.\n";

    LOG(INFO) << "Finished Vulkan initialization.\n";
    return true;
}

bool MightyEngine::createSyncObjects() {
    presentCompleteSemaphore_ =
        logicalDevice_.createSemaphore(vk::SemaphoreCreateInfo()).value;

    renderFinishedSemaphore_ =
        logicalDevice_.createSemaphore(vk::SemaphoreCreateInfo()).value;
    drawFence_ = logicalDevice_
                     .createFence({.flags = vk::FenceCreateFlagBits::eSignaled})
                     .value;

    return true;
}

// At a high level, rendering a frame in Vulkan consists of a common set of
// steps:
// * Wait for the previous frame to finish
// * Acquire an image from the swap chain
// * Record a command buffer which draws the scene onto that image
// * Submit the recorded command buffer
// * Present the swap chain image
bool MightyEngine::drawFrame() {
    auto [result, imageIndex] = swapChain_.acquireNextImage(UINT64_MAX,
        *presentCompleteSemaphore_,
        nullptr);
    recordCommandBuffer(imageIndex);

    logicalDevice_.resetFences(*drawFence_);

    vk::PipelineStageFlags waitDestinationStageMask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo{.waitSemaphoreCount = 1,
        .pWaitSemaphores = &*presentCompleteSemaphore_,
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffer_,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*renderFinishedSemaphore_};

    deviceQueue_.submit(submitInfo, *drawFence_);

    while (vk::Result::eTimeout
           == logicalDevice_.waitForFences(*drawFence_, vk::True, UINT64_MAX))
        ;

    const vk::PresentInfoKHR presentInfoKHR{.waitSemaphoreCount = 1,
        .pWaitSemaphores = &*renderFinishedSemaphore_,
        .swapchainCount = 1,
        .pSwapchains = &*swapChain_,
        .pImageIndices = &imageIndex};
    result = deviceQueue_.presentKHR(presentInfoKHR);
    switch (result) {
        case vk::Result::eSuccess:
            break;
        case vk::Result::eSuboptimalKHR:
            std::cout << "vk::Queue::presentKHR returned "
                         "vk::Result::eSuboptimalKHR !\n";
            break;
        default:
            break;  // an unexpected result is returned!
    }
    return true;
}

[[nodiscard]] vk::raii::ShaderModule MightyEngine::createShaderModule(
    const std::vector<char>& code) const {
    vk::ShaderModuleCreateInfo createInfo{.codeSize =
                                              code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())};
    return logicalDevice_.createShaderModule(createInfo).value;
}

bool MightyEngine::createCommandBuffer() {
    vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool_,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1};
    commandBuffer_ = std::move(
        logicalDevice_.allocateCommandBuffers(allocInfo).value.front());
    return true;
}
bool MightyEngine::createImageViews() {
    vk::ImageViewCreateInfo imageViewCreateInfo{.viewType =
                                                    vk::ImageViewType::e2D,
        .format = swapChainSurfaceFormat_.format,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
    for (auto image : swapChainImages_) {
        imageViewCreateInfo.image = image;
        auto [result, value] =
            logicalDevice_.createImageView(imageViewCreateInfo);
        if (result != vk::Result::eSuccess) {
            return false;
        }
        swapChainImageViews_.emplace_back(std::move(value));
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

bool MightyEngine::createCommandPool() {
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphicsIndex_};

    auto [result, commandPool] = logicalDevice_.createCommandPool(poolInfo);
    if (result != vk::Result::eSuccess) {
        return false;
    }
    commandPool_ = std::move(commandPool);
    return true;
}

bool MightyEngine::createSwapChain() {
    auto [result, surfaceCapabilities] =
        physicalDevice_.getSurfaceCapabilitiesKHR(surface_);
    if (result != vk::Result::eSuccess) {
        return false;
    }
    auto [surfaceResult, surfaceFormat] =
        physicalDevice_.getSurfaceFormatsKHR(surface_);
    if (surfaceResult != vk::Result::eSuccess) {
        return false;
    }
    swapChainSurfaceFormat_ = chooseSwapSurfaceFormat(surfaceFormat);
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
    auto [presentModeResult, presentMode] =
        physicalDevice_.getSurfacePresentModesKHR(surface_);
    if (presentModeResult != vk::Result::eSuccess) {
        return false;
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
        .presentMode = chooseSwapPresentMode(presentMode),
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
    auto [swapChainResult, swapChain] =
        logicalDevice_.createSwapchainKHR(swapChainCreateInfo);
    if (swapChainResult != vk::Result::eSuccess) {
        return false;
    }
    swapChain_ = std::move(swapChain);

    auto [swapChainImagesResult, swapChainImages] = swapChain_.getImages();
    if (swapChainImagesResult != vk::Result::eSuccess) {
        return false;
    }
    swapChainImages_ = swapChainImages;

    return true;
}

bool MightyEngine::createGraphicsPipeline() {
    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(kDynamicStates.size()),
        .pDynamicStates = kDynamicStates.data()};
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = vk::PrimitiveTopology::eTriangleList};
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

    auto [graphicsPipelineLayoutResult, graphicsPipelineLayout] =
        logicalDevice_.createPipelineLayout(pipelineLayoutInfo);
    if (graphicsPipelineLayoutResult != vk::Result::eSuccess) {
        return false;
    }
    graphicsPipelineLayout_ = std::move(graphicsPipelineLayout);
    // Create shaders
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

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
        fragShaderStageInfo};
    // End of create shaders

    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapChainSurfaceFormat_.format};
    vk::GraphicsPipelineCreateInfo pipelineInfo{
        .pNext = &pipelineRenderingCreateInfo,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = graphicsPipelineLayout_,
        .renderPass = nullptr};

    auto [graphicsPipelineResult, graphicsPipeline] =
        logicalDevice_.createGraphicsPipeline(nullptr, pipelineInfo);
    if (graphicsPipelineResult != vk::Result::eSuccess) {
        return false;
    }
    graphicsPipeline_ = std::move(graphicsPipeline);
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