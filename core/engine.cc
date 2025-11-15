#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_EXCEPTIONS
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#define STB_IMAGE_IMPLEMENTATION

#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
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
#include "utils.h"

namespace core {

void MightyEngine::recordCommandBuffer(uint32_t imageIndex) {
    vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    commandBuffer_.begin(beginInfo);

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

void MightyEngine::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(kWindowWidth,
        kWindowHeight,
        kAppName.data(),
        nullptr,
        nullptr);

    setWindowIcon(window_, "assets/ICON.png");
}
void MightyEngine::loop() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        drawFrame();
    }
    logicalDevice_.waitIdle();
}
void MightyEngine::initVK() {
    LOG(INFO) << "Initilazing " << kAppName << "\n";
    LOG(INFO) << "Enable Validation Layers:\n"
              << kEnableValidationLayers << "\n"
              << "More debugging:" << kMoreLogs << "\n";
    createVKInstance();
    setupDebugMessenger();
    createSurface();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();

    LOG(INFO) << "Finished Vulkan initialization.\n";
}

// At a high level, rendering a frame in Vulkan consists of a common set of
// steps:
// * Wait for the previous frame to finish
// * Acquire an image from the swap chain
// * Record a command buffer which draws the scene onto that image
// * Submit the recorded command buffer
// * Present the swap chain image
void MightyEngine::drawFrame() {
    auto imageIndex =
        swapChain_
            .acquireNextImage(UINT64_MAX, *presentCompleteSemaphore_, nullptr)
            .value;
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
    auto result = deviceQueue_.presentKHR(presentInfoKHR);

    if (result == vk::Result::eSuboptimalKHR) {
        LOG(WARR) << "vk::Queue::presentKHR returned "
                     "vk::Result::eSuboptimalKHR !\n";
    }
}

[[nodiscard]] vk::raii::ShaderModule MightyEngine::createShaderModule(
    const std::vector<char>& code) const {
    vk::ShaderModuleCreateInfo createInfo{.codeSize =
                                              code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())};
    return logicalDevice_.createShaderModule(createInfo).value;
}

void MightyEngine::createImageViews() {
    vk::ImageViewCreateInfo imageViewCreateInfo{.viewType =
                                                    vk::ImageViewType::e2D,
        .format = swapChainSurfaceFormat_.format,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
    for (auto image : swapChainImages_) {
        imageViewCreateInfo.image = image;
        auto imageView =
            logicalDevice_.createImageView(imageViewCreateInfo).value;
        swapChainImageViews_.emplace_back(std::move(imageView));
    }
}

void MightyEngine::createSurface() {
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(*instance_, window_, nullptr, &surface);
    surface_ = vk::raii::SurfaceKHR(instance_, surface);
}

void MightyEngine::createSwapChain() {
    auto surfaceCapabilities =
        physicalDevice_.getSurfaceCapabilitiesKHR(surface_).value;
    swapChainSurfaceFormat_ =
        vk::SurfaceFormatKHR{.format = vk::Format::eB8G8R8A8Srgb,
            .colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear};
    swapChainExtent_ = surfaceCapabilities.currentExtent;

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .flags = vk::SwapchainCreateFlagsKHR(),
        .surface = surface_,
        .minImageCount = surfaceCapabilities.minImageCount + 1,
        .imageFormat = swapChainSurfaceFormat_.format,
        .imageColorSpace = swapChainSurfaceFormat_.colorSpace,
        .imageExtent = swapChainExtent_,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = vk::PresentModeKHR::eMailbox,
        .clipped = true,
        .oldSwapchain = nullptr};

    swapChain_ = logicalDevice_.createSwapchainKHR(swapChainCreateInfo).value;
    swapChainImages_ = swapChain_.getImages().value;
}

void MightyEngine::createGraphicsPipeline() {
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

    graphicsPipelineLayout_ =
        logicalDevice_.createPipelineLayout(pipelineLayoutInfo).value;
    // Create shaders
    auto shaderPath = getExecutableDir() / "shaders" / "slang.spv";
    auto shaderCode = readFile(shaderPath.string());
    if (!shaderCode.has_value()) {
        LOG(ERR) << "Can't find shader file location.\n";
        return;
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

    graphicsPipeline_ =
        logicalDevice_.createGraphicsPipeline(nullptr, pipelineInfo).value;

    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphicsQueueFamilyIndex_};
    commandPool_ = logicalDevice_.createCommandPool(poolInfo).value;

    vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool_,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1};
    commandBuffer_ = std::move(
        logicalDevice_.allocateCommandBuffers(allocInfo).value.front());

    presentCompleteSemaphore_ =
        logicalDevice_.createSemaphore(vk::SemaphoreCreateInfo()).value;

    renderFinishedSemaphore_ =
        logicalDevice_.createSemaphore(vk::SemaphoreCreateInfo()).value;
    drawFence_ = logicalDevice_
                     .createFence({.flags = vk::FenceCreateFlagBits::eSignaled})
                     .value;
}

void MightyEngine::createLogicalDevice() {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        physicalDevice_.getQueueFamilyProperties();

    float queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = graphicsQueueFamilyIndex_,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority};

    vk::StructureChain<vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        featureChain = {{},
            {.shaderDrawParameters = true},
            {.synchronization2 = true, .dynamicRendering = true},
            {.extendedDynamicState = true}};
    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount =
            static_cast<uint32_t>(kDeviceExtensions.size()),
        .ppEnabledExtensionNames = kDeviceExtensions.data()};

    logicalDevice_ = physicalDevice_.createDevice(deviceCreateInfo).value;
    deviceQueue_ = logicalDevice_.getQueue(graphicsQueueFamilyIndex_, 0);
    presentQueue_ = logicalDevice_.getQueue(graphicsQueueFamilyIndex_, 0);
}

void MightyEngine::setupDebugMessenger() {
    if (!kEnableValidationLayers) {
        LOG(INFO)
            << "Debug Messenger is not initialized. No Validation Layers.\n";
        return;
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

    debugMessenger_ =
        instance_.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT)
            .value;
}

void MightyEngine::createVKInstance() {
    constexpr vk::ApplicationInfo appInfo{.pApplicationName = kAppName.data(),
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = kAppName.data(),
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion14};

    std::vector<char const*> requiredLayers;
    if (kEnableValidationLayers) {
        requiredLayers.assign(kValidationLayers.begin(),
            kValidationLayers.end());
    }

    auto extensions = getInstanceExtensions();

    vk::InstanceCreateInfo createInfo{.pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()};

    instance_ = context_.createInstance(createInfo).value;
    physicalDevice_ = instance_.enumeratePhysicalDevices().value[0];
}

void MightyEngine::cleanup() {
    glfwDestroyWindow(window_);
    glfwTerminate();
}

}  // namespace core