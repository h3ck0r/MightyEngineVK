#define STB_IMAGE_IMPLEMENTATION

#include "engine.h"

#include <GLFW/glfw3native.h>
#include <windows.h>

#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/trigonometric.hpp"
#include "globals.h"
#include "utils.h"
#include "vulkan/vulkan.hpp"

namespace core {

MightyAccelerationStruct MightyEngine::createAccelerationStruct(
    vk::AccelerationStructureGeometryKHR geometry,
    uint32_t primitiveCount,
    vk::AccelerationStructureTypeKHR type) {
    MightyAccelerationStruct accel;
    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{.type =
                                                                        type,
        .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
        .pGeometries = &geometry};

    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo =
        logicalDevice.getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice,
            buildGeometryInfo,
            primitiveCount);
    vk::DeviceSize size = buildSizesInfo.accelerationStructureSize;
    accel.buffer = createBuffer(MightyBuffer::Type::AccelerationStorage, size);

    accel.accelerationStruct =
        logicalDevice
            .createAccelerationStructureKHR(
                {.buffer = accel.buffer.buffer, .size = size, .type = type})
            .value;

    MightyBuffer scratchBuffer = createBuffer(MightyBuffer::Type::Scratch,
        buildSizesInfo.buildScratchSize);
    
        // buildGeometryInfo.setScratchData(scratchBuffer.deviceAddress);
         

    return accel;
}

MightyBuffer MightyEngine::createBuffer(MightyBuffer::Type type,
    vk::DeviceSize size,
    const void* data) {
    MightyBuffer mightyBuffer;
    vk::BufferUsageFlags usage;
    vk::MemoryPropertyFlags memoryProps;

    if (type == MightyBuffer::Type::AccelerationInput) {
        usage =
            vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
            | vk::BufferUsageFlagBits::eStorageBuffer
            | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        memoryProps = vk::MemoryPropertyFlagBits::eHostVisible
                      | vk::MemoryPropertyFlagBits::eHostCoherent;
    } else if (type == MightyBuffer::Type::Scratch) {
        usage = vk::BufferUsageFlagBits::eStorageBuffer
                | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        memoryProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
    } else if (type == MightyBuffer::Type::AccelerationStorage) {
        usage = vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR
                | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        memoryProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
    } else if (type == MightyBuffer::Type::ShaderBindingTable) {
        usage = vk::BufferUsageFlagBits::eShaderBindingTableKHR
                | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        memoryProps = vk::MemoryPropertyFlagBits::eHostVisible
                      | vk::MemoryPropertyFlagBits::eHostCoherent;
    }

    mightyBuffer.buffer =
        logicalDevice.createBuffer({.size = size, .usage = usage}).value;

    vk::MemoryRequirements memRequirements =
        mightyBuffer.buffer.getMemoryRequirements();

    vk::MemoryAllocateFlagsInfo flagsInfo{
        .flags = vk::MemoryAllocateFlagBits::eDeviceAddress};

    vk::MemoryAllocateInfo allocInfo{.pNext = &flagsInfo,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(physicalDevice,
            memRequirements.memoryTypeBits,
            memoryProps)};

    mightyBuffer.memory = logicalDevice.allocateMemory(allocInfo).value;
    mightyBuffer.buffer.bindMemory(*mightyBuffer.memory, 0);

    vk::BufferDeviceAddressInfoKHR bufferDeviceAI{
        .buffer = mightyBuffer.buffer};
    mightyBuffer.deviceAddress =
        logicalDevice.getBufferAddressKHR(bufferDeviceAI);

    mightyBuffer.descriptorBufferInfo = {.buffer = mightyBuffer.buffer,
        .offset = 0,
        .range = size};
    if (data) {
        void* mapped = mightyBuffer.memory.mapMemory(0, size).value;
        memcpy(mapped, data, size);
        mightyBuffer.memory.unmapMemory();
    }
    return mightyBuffer;
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
    currentCommandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
        graphicsPipelineLayout,
        0,
        *descriptorSets[currentFrame],
        nullptr);
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
    currentCommandBuffer->bindVertexBuffers(0, *vertexBuffer.buffer, {0});
    currentCommandBuffer->bindIndexBuffer(*indexBuffer.buffer,
        0,
        vk::IndexType::eUint32);
    currentCommandBuffer->drawIndexed(indices.size(), 1, 0, 0, 0);
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
    initVK();
    loop();
    cleanup();
}

void MightyEngine::loop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }
    logicalDevice.waitIdle();
}
void MightyEngine::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
        currentTime - startTime)
                     .count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f),
        time * glm::radians(90.f),
        glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f),
        static_cast<float>(swapChainExtent.width)
            / static_cast<float>(swapChainExtent.height),
        0.1f,
        10.0f);

    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
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
    while (logicalDevice.waitForFences(*inFlightFences[currentFrame],
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

    logicalDevice.resetFences(*inFlightFences[currentFrame]);
    commandBuffers[currentFrame].reset();
    recordCommandBuffer(imageIndex);

    updateUniformBuffer(currentFrame);

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
    return logicalDevice.createShaderModule(createInfo).value;
}

void MightyEngine::recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    logicalDevice.waitIdle();

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
        auto imageView =
            logicalDevice.createImageView(imageViewCreateInfo).value;
        swapChainImageViews.emplace_back(std::move(imageView));
    }
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

    swapChain = logicalDevice.createSwapchainKHR(swapChainCreateInfo).value;
    swapChainImages = swapChain.getImages().value;
}

void MightyEngine::copyBuffer(vk::raii::Buffer& srcBuffer,
    vk::raii::Buffer& dstBuffer,
    vk::DeviceSize size) {
    vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1};
    vk::raii::CommandBuffer commandCopyBuffer =
        std::move(logicalDevice.allocateCommandBuffers(allocInfo)->front());
    commandCopyBuffer.begin(
        {.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    commandCopyBuffer.copyBuffer(srcBuffer,
        dstBuffer,
        vk::BufferCopy(0, 0, size));
    commandCopyBuffer.end();

    deviceQueue.submit(vk::SubmitInfo{.commandBufferCount = 1,
        .pCommandBuffers = &*commandCopyBuffer});
    deviceQueue.waitIdle();
}

void MightyEngine::cleanup() {
    cleanupSwapChain();
    glfwDestroyWindow(window);
    glfwTerminate();
}

}  // namespace core