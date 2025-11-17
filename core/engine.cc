#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_EXCEPTIONS
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#define STB_IMAGE_IMPLEMENTATION
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <windows.h>

#include <glm/glm.hpp>

#include "engine.h"
#include "globals.h"
#include "utils.h"

namespace core {
const std::vector<Vertex> vertices = {{{0.0f, -0.5f}, {1.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};

static void frameBufferResizeCallback(GLFWwindow* window,
    int width,
    int height) {
    auto app =
        reinterpret_cast<MightyEngine*>(glfwGetWindowUserPointer(window));
    app->frameBufferResized = true;
}

void MightyEngine::recordCommandBuffer(uint32_t imageIndex) {
    vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    auto currentCommandBuffer = &commandBuffers[currentFrame];
    currentCommandBuffer->begin(beginInfo);

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
        .imageView = swapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = {clearColor}};

    vk::RenderingInfo renderingInfo = {.renderArea = {.offset = {0, 0},
                                           .extent = swapChainExtent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo};

    currentCommandBuffer->beginRendering(renderingInfo);
    currentCommandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics,
        graphicsPipeline);
    currentCommandBuffer->setViewport(0,
        vk::Viewport(0.0f,
            0.0f,
            static_cast<float>(swapChainExtent.width),
            static_cast<float>(swapChainExtent.height),
            0.0f,
            1.0f));
    currentCommandBuffer->setScissor(0,
        vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));
    currentCommandBuffer->bindVertexBuffers(0, *vertexBuffer, {0});
    currentCommandBuffer->draw(3, 1, 0, 0);
    currentCommandBuffer->endRendering();

    transitioImageLayout(imageIndex,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,          // srcAccessMask
        {},                                                  // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,  // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe            // dstStage
    );
    currentCommandBuffer->end();
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
        .image = swapChainImages[imageIndex],
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1}};
    vk::DependencyInfo dependencyInfo = {.dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier};
    commandBuffers[currentFrame].pipelineBarrier2(dependencyInfo);
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

    window = glfwCreateWindow(kWindowWidth,
        kWindowHeight,
        kAppName.data(),
        nullptr,
        nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, frameBufferResizeCallback);
    setWindowIcon(window, "assets/ICON.png");
}

void MightyEngine::loop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }
    device.waitIdle();
}
void MightyEngine::initVK() {
    LOG(INFO) << "Initilazing " << kAppName << "\n";

    createVKInstance();
    setupDebugMessenger();
    createSurface();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
    createVertexBuffer();

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
    if (frameBufferResized) {
        recreateSwapChain();
        frameBufferResized = false;
        return;
    }
    while (device.waitForFences(*inFlightFences[currentFrame],
               vk::True,
               UINT64_MAX)
           == vk::Result::eTimeout)
        ;

    auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX,
        *presentCompleteSemaphores[semaphoreIndex],
        nullptr);
    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapChain();
        return;
    } else if (result != vk::Result::eSuccess
               && result != vk::Result::eSuboptimalKHR) {
        return;
    }

    device.resetFences(*inFlightFences[currentFrame]);
    commandBuffers[currentFrame].reset();
    recordCommandBuffer(imageIndex);

    vk::PipelineStageFlags waitDestinationStageMask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo{.waitSemaphoreCount = 1,
        .pWaitSemaphores = &*presentCompleteSemaphores[semaphoreIndex],
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffers[currentFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*renderFinishedSemaphores[imageIndex]};

    deviceQueue.submit(submitInfo, inFlightFences[currentFrame]);

    const vk::PresentInfoKHR presentInfoKHR{.waitSemaphoreCount = 1,
        .pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &*swapChain,
        .pImageIndices = &imageIndex};
    auto presentResult = deviceQueue.presentKHR(presentInfoKHR);
    if (presentResult == vk::Result::eErrorOutOfDateKHR
        || presentResult == vk::Result::eSuboptimalKHR) {
        recreateSwapChain();
    }

    semaphoreIndex = (semaphoreIndex + 1) % presentCompleteSemaphores.size();
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

[[nodiscard]] vk::raii::ShaderModule MightyEngine::createShaderModule(
    const std::vector<char>& code) const {
    vk::ShaderModuleCreateInfo createInfo{.codeSize =
                                              code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())};
    return device.createShaderModule(createInfo).value;
}

void MightyEngine::recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    device.waitIdle();

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
}

void MightyEngine::cleanupSwapChain() {
    swapChainImageViews.clear();
    swapChain = nullptr;
}

void MightyEngine::createImageViews() {
    vk::ImageViewCreateInfo imageViewCreateInfo{.viewType =
                                                    vk::ImageViewType::e2D,
        .format = swapChainSurfaceFormat.format,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
    for (auto image : swapChainImages) {
        imageViewCreateInfo.image = image;
        auto imageView = device.createImageView(imageViewCreateInfo).value;
        swapChainImageViews.emplace_back(std::move(imageView));
    }
}

void MightyEngine::createSurface() {
    VkSurfaceKHR newSurface;
    glfwCreateWindowSurface(*instance, window, nullptr, &newSurface);
    surface = vk::raii::SurfaceKHR(instance, newSurface);
}

void MightyEngine::createSwapChain() {
    auto surfaceCapabilities =
        physicalDevice.getSurfaceCapabilitiesKHR(surface).value;
    swapChainSurfaceFormat =
        vk::SurfaceFormatKHR{.format = vk::Format::eB8G8R8A8Srgb,
            .colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear};
    swapChainExtent = surfaceCapabilities.currentExtent;

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .flags = vk::SwapchainCreateFlagsKHR(),
        .surface = surface,
        .minImageCount = MAX_FRAMES_IN_FLIGHT,
        .imageFormat = swapChainSurfaceFormat.format,
        .imageColorSpace = swapChainSurfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = vk::PresentModeKHR::eMailbox,
        .clipped = true,
        .oldSwapchain = nullptr};

    swapChain = device.createSwapchainKHR(swapChainCreateInfo).value;
    swapChainImages = swapChain.getImages().value;
}

uint32_t MightyEngine::findMemoryType(uint32_t typeFilter,
    vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties =
        physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i))
            && (memProperties.memoryTypes[i].propertyFlags & properties)
                   == properties) {
            return i;
        }
    }
    return -1;
}

void MightyEngine::createVertexBuffer() {
    vk::BufferCreateInfo bufferInfo{.size =
                                        sizeof(vertices[0]) * vertices.size(),
        .usage = vk::BufferUsageFlagBits::eVertexBuffer,
        .sharingMode = vk::SharingMode::eExclusive};

    vertexBuffer = device.createBuffer(bufferInfo).value;
    vk::MemoryRequirements memRequirements =
        vertexBuffer.getMemoryRequirements();
    vk::MemoryAllocateInfo memoryAllocateInfo{.allocationSize =
                                                  memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible
                | vk::MemoryPropertyFlagBits::eHostCoherent)};
    vertexBufferMemory = device.allocateMemory(memoryAllocateInfo).value;
    vertexBuffer.bindMemory(*vertexBufferMemory, 0);

    void* data = vertexBufferMemory.mapMemory(0, bufferInfo.size).value;
    memcpy(data, vertices.data(), bufferInfo.size);
    vertexBufferMemory.unmapMemory();
}

void MightyEngine::createGraphicsPipeline() {
    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(kDynamicStates.size()),
        .pDynamicStates = kDynamicStates.data()};

    auto bindingDescription = getBindingDescription();
    auto attributeDescriptions = getAttributeDescriptor();
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = attributeDescriptions.size(),
        .pVertexAttributeDescriptions = attributeDescriptions.data()};

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

    graphicsPipelineLayout =
        device.createPipelineLayout(pipelineLayoutInfo).value;
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
        .pColorAttachmentFormats = &swapChainSurfaceFormat.format};
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
        .layout = graphicsPipelineLayout,
        .renderPass = nullptr};

    graphicsPipeline =
        device.createGraphicsPipeline(nullptr, pipelineInfo).value;

    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphicsQueueFamilyIndex};
    commandPool = device.createCommandPool(poolInfo).value;

    vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT};
    commandBuffers = device.allocateCommandBuffers(allocInfo).value;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        presentCompleteSemaphores.emplace_back(
            device.createSemaphore(vk::SemaphoreCreateInfo()).value);
        renderFinishedSemaphores.emplace_back(
            device.createSemaphore(vk::SemaphoreCreateInfo()).value);
        inFlightFences.emplace_back(
            device.createFence({.flags = vk::FenceCreateFlagBits::eSignaled})
                .value);
    }
}

void MightyEngine::createLogicalDevice() {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        physicalDevice.getQueueFamilyProperties();

    float queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = graphicsQueueFamilyIndex,
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

    device = physicalDevice.createDevice(deviceCreateInfo).value;
    deviceQueue = device.getQueue(graphicsQueueFamilyIndex, 0);
    presentQueue = device.getQueue(graphicsQueueFamilyIndex, 0);
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

    debugMessenger =
        instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT)
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

    instance = context.createInstance(createInfo).value;
    physicalDevice = instance.enumeratePhysicalDevices().value[0];
}

void MightyEngine::cleanup() {
    cleanupSwapChain();

    glfwDestroyWindow(window);
    glfwTerminate();
}

}  // namespace core