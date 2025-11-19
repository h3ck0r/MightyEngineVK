#include <cstdint>

#include "engine.h"
#include "globals.h"
#include "utils.h"
#include "vulkan/vulkan.hpp"

namespace core {
static void frameBufferResizeCallback(GLFWwindow* window,
    int width,
    int height) {
    auto app =
        reinterpret_cast<MightyEngine*>(glfwGetWindowUserPointer(window));
    app->frameBufferResized = true;
}
void MightyEngine::initVK() {
    LOG(INFO) << "Initilazing " << kAppName << "\n";

    // CREATE WINDOW
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
    // END OF CREATE WINDOW
    // VK INSTANCE
    constexpr vk::ApplicationInfo appInfo{.pApplicationName = kAppName.data(),
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = kAppName.data(),
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion14};

    std::vector<char const*> requiredLayers;
    requiredLayers.assign(kValidationLayers.begin(), kValidationLayers.end());

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions =
        glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions,
        glfwExtensions + glfwExtensionCount);

    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    vk::InstanceCreateInfo createInfo{.pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()};

    instance = context.createInstance(createInfo).value;
    physicalDevice = instance.enumeratePhysicalDevices().value[0];
    // END OF VK INSTANCE
    // DEBUGGER
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
    // END OF DEBUGGER
    // SURFACE
    VkSurfaceKHR newSurface;
    glfwCreateWindowSurface(*instance, window, nullptr, &newSurface);
    surface = vk::raii::SurfaceKHR(instance, newSurface);
    // END OF SURFACE
    // LOGICAL DEVICE
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        physicalDevice.getQueueFamilyProperties();

    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = graphicsQueueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority};

    vk::StructureChain<vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
        vk::PhysicalDeviceBufferDeviceAddressFeatures,
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>
        featureChain = {{},
            {.shaderDrawParameters = true},
            {.synchronization2 = true, .dynamicRendering = true},
            {.extendedDynamicState = true},
            {.bufferDeviceAddress = true},
            {.accelerationStructure = true},
            {.rayTracingPipeline = true}};
    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount =
            static_cast<uint32_t>(kDeviceExtensions.size()),
        .ppEnabledExtensionNames = kDeviceExtensions.data()};

    logicalDevice = physicalDevice.createDevice(deviceCreateInfo).value;
    deviceQueue = logicalDevice.getQueue(graphicsQueueFamilyIndex, 0);
    presentQueue = logicalDevice.getQueue(graphicsQueueFamilyIndex, 0);
    // END OF LOGICAL DEVICE
    // SWAPCHAIN
    createSwapChain();
    // IMAGE VIEWS
    createImageViews();
    // LOAD MODEL
    loadFromFile(vertices, indices, materials);
    // DESCRIPTOR SET LAYOUT
    vk::DescriptorSetLayoutBinding uboLayoutBinding{0,
        vk::DescriptorType::eUniformBuffer,
        1,
        vk::ShaderStageFlagBits::eVertex,
        nullptr};
    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .flags = {},
        .bindingCount = 1,
        .pBindings = &uboLayoutBinding,
    };
    descriptorSetLayout =
        logicalDevice.createDescriptorSetLayout(layoutInfo).value;
    // END OF DESCRIPTOR SET LAYOUT
    // GRAPHICS PIPELINE
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
        .frontFace = vk::FrontFace::eCounterClockwise,
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

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{.setLayoutCount = 1,
        .pSetLayouts = &*descriptorSetLayout,
        .pushConstantRangeCount = 0};

    graphicsPipelineLayout =
        logicalDevice.createPipelineLayout(pipelineLayoutInfo).value;
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
        logicalDevice.createGraphicsPipeline(nullptr, pipelineInfo).value;

    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphicsQueueFamilyIndex};
    commandPool = logicalDevice.createCommandPool(poolInfo).value;

    vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT};
    commandBuffers = logicalDevice.allocateCommandBuffers(allocInfo).value;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        presentCompleteSemaphores.emplace_back(
            logicalDevice.createSemaphore(vk::SemaphoreCreateInfo()).value);
        renderFinishedSemaphores.emplace_back(
            logicalDevice.createSemaphore(vk::SemaphoreCreateInfo()).value);
        inFlightFences.emplace_back(logicalDevice
                .createFence({.flags = vk::FenceCreateFlagBits::eSignaled})
                .value);
    }
    // END OF GRAPHICS PIPELINE
    // BUFFERS
    vertexBuffer = createBuffer(MightyBuffer::Type::AccelerationInput,
        sizeof(Vertex) * vertices.size(),
        vertices.data());

    indexBuffer = createBuffer(MightyBuffer::Type::AccelerationInput,
        sizeof(uint32_t) * indices.size(),
        vertices.data());

    materialBuffer = createBuffer(MightyBuffer::Type::AccelerationInput,
        sizeof(Material) * materials.size(),
        materials.data());
    // END OF BUFFERS
    // BLAS
    vk::AccelerationStructureGeometryTrianglesDataKHR triangleData{
        .vertexFormat = vk::Format::eR32G32B32Sfloat,
        .vertexData = {vertexBuffer.deviceAddress},
        .vertexStride = sizeof(Vertex),
        .maxVertex = static_cast<uint32_t>(vertices.size()),
        .indexType = vk::IndexType::eUint32,
        .indexData = {indexBuffer.deviceAddress}};

    vk::AccelerationStructureGeometryKHR triangleGeometry{
        .geometryType = vk::GeometryTypeKHR::eTriangles,
        .geometry = {triangleData},
        .flags = vk::GeometryFlagBitsKHR::eOpaque};
    const auto primitiveCount = static_cast<uint32_t>(indices.size() / 3);
    // END OF BLAS
    // UNIFORM BUFFER
    // uniformBuffers.clear();
    // uniformBuffersMemory.clear();
    // uniformBuffersMapped.clear();

    // for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    //     vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
    //     vk::raii::Buffer buffer = nullptr;
    //     vk::raii::DeviceMemory bufferMemory = nullptr;
    //     createBuffer(bufferSize,
    //         vk::BufferUsageFlagBits::eUniformBuffer,
    //         vk::MemoryPropertyFlagBits::eHostVisible
    //             | vk::MemoryPropertyFlagBits::eHostCoherent,
    //         buffer,
    //         bufferMemory);
    //     uniformBuffers.emplace_back(std::move(buffer));
    //     uniformBuffersMemory.emplace_back(std::move(bufferMemory));
    //     uniformBuffersMapped.emplace_back(
    //         uniformBuffersMemory[i].mapMemory(0, bufferSize).value);
    // }
    // END OF UNIFORM BUFFER
    // DESCRIPTOR POOL
    std::vector<vk::DescriptorPoolSize> poolSize{
        {vk::DescriptorType::eAccelerationStructureKHR, 1},
        {vk::DescriptorType::eStorageImage, 1},
        {vk::DescriptorType::eStorageBuffer, 3},
    };
    vk::DescriptorPoolCreateInfo descriptorPoolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = 1,
        .pPoolSizes = poolSize.data()};

    descriptorPool =
        logicalDevice.createDescriptorPool(descriptorPoolInfo).value;
    // END OF DESCRIPTOR POOL
    // DESCRIPTOR SETS
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
        *descriptorSetLayout);
    vk::DescriptorSetAllocateInfo descriptorSetInifInfo{.descriptorPool =
                                                            descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()};
    descriptorSets.clear();
    descriptorSets =
        logicalDevice.allocateDescriptorSets(descriptorSetInifInfo).value;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo bufferInfo{.buffer = uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject)};
        vk::WriteDescriptorSet descriptorWrite{.dstSet = descriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &bufferInfo};
        logicalDevice.updateDescriptorSets(descriptorWrite, {});
    }
    // END OF DESCRIPTOR SETS

    LOG(INFO) << "Finished Vulkan initialization.\n";
}

}  // namespace core