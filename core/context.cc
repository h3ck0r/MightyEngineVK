#include "context.h"

#include <vulkan/vulkan_core.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define STB_IMAGE_IMPLEMENTATION
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <stb_image.h>
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
    initWindow();
    createInstance();
    createDeviceAndQueue();
    createSurfaceAndSwapchain();
    createCommandPoolAndDescriptorPool();
    createRayTraycingPipeline();
    loop();
    cleanup();
}

void MtyContext::createCommandPoolAndDescriptorPool() {
    VkCommandPoolCreateInfo commandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VkCommandPoolCreateFlagBits::
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndex,
    };
    vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);

    // If need new shader data add here desc
    std::vector<VkDescriptorPoolSize> descriptorPoolSizes = {
        {VkDescriptorType::VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            1},  // FOr TLAS
        {VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
        {VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VkDescriptorPoolCreateFlagBits::
            VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1,
        .poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size()),
        .pPoolSizes = descriptorPoolSizes.data()};
    vkCreateDescriptorPool(device,
        &descriptorPoolCreateInfo,
        nullptr,
        &descriptorPool);
}

void MtyContext::createRayTraycingPipeline() {
    std::vector<std::vector<char>> modulesCode = {readFile(
                                                      "./shaders/raygen.spv"),
        readFile("./shaders/miss.spv"),
        readFile("./shaders/closesthit.spv")};

    for (const auto& code : modulesCode) {
        VkShaderModule shaderModule;
        VkShaderModuleCreateInfo shaderModuleCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.size(),
            .pCode = reinterpret_cast<const uint32_t*>(code.data())};
        vkCreateShaderModule(device,
            &shaderModuleCreateInfo,
            nullptr,
            &shaderModule);
        shaderModules.push_back(shaderModule);
    }

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR,
            .module = shaderModules[0],
            .pName = "main"},
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR,
            .module = shaderModules[1],
            .pName = "main"},
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            .module = shaderModules[2],
            .pName = "main"}};

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{
        VkRayTracingShaderGroupCreateInfoKHR{
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type = VkRayTracingShaderGroupTypeKHR::
                VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
            .generalShader = 0,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR},
        VkRayTracingShaderGroupCreateInfoKHR{
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type = VkRayTracingShaderGroupTypeKHR::
                VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
            .generalShader = 1,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR},
        VkRayTracingShaderGroupCreateInfoKHR{
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type = VkRayTracingShaderGroupTypeKHR::
                VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
            .generalShader = VK_SHADER_UNUSED_KHR,
            .closestHitShader = 2,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR}};

    std::vector<VkDescriptorSetLayoutBinding> bindings{
        // 0 = TLAS
        VkDescriptorSetLayoutBinding{.binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .descriptorCount = 1,
            .stageFlags =
                VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR},
        // 1 = Storage Image
        VkDescriptorSetLayoutBinding{.binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags =
                VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR},
        // 2 = Vertices
        VkDescriptorSetLayoutBinding{.binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags =
                VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR},
        // 3 = Indices
        VkDescriptorSetLayoutBinding{.binding = 3,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags =
                VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR},
        // 4 = Material
        VkDescriptorSetLayoutBinding{.binding = 4,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags =
                VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR},
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()};
    vkCreateDescriptorSetLayout(device,
        &descriptorSetLayoutCreateInfo,
        nullptr,
        &descriptorSetLayout);

    VkPushConstantRange pushRange{
        .stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        .offset = 0,
        .size = sizeof(int)};

    // VkRayTracingPipelineCreateInfoKHR raytracingPipelineCreateInfo{
    //     .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
    //     .stageCount = static_cast<uint32_t>(shaderStages.size()),
    //     .pStages = shaderStages.data(),
    //     .groupCount = static_cast<uint32_t>(shaderGroups.size()),
    //     .pGroups = shaderGroups.data(),
    //     .layout = descriptorSetLayout,
    //     .maxPipelineRayRecursionDepth =
    //         4,  // change this to a higher or lower values

    // };
    // vkCreateRayTracingPipelinesKHR(device,
    //     VkDeferredOperationKHR deferredOperation,
    //     VkPipelineCache pipelineCache,
    //     1,
    //     const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
    //     const VkAllocationCallbacks* pAllocator,
    //     VkPipeline* pPipelines)
}

void MtyContext::loop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void MtyContext::initWindow() {
    int width, height, channels;
    unsigned char* pixels =
        stbi_load("assets/ICON.png", &width, &height, &channels, 4);

    GLFWimage image;
    image.height = height;
    image.width = width;
    image.pixels = pixels;

    glfwInit();
    glfwSetErrorCallback(windowErrorCallback);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(WINDOW_WIDTH,
        WINDOW_HEIGHT,
        "Mighty Engine",
        nullptr,
        nullptr);
    glfwSetWindowIcon(window, 1, &image);
}

void MtyContext::createSurfaceAndSwapchain() {
    MTY_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));

    VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
        .surface = surface};
    VkSurfaceCapabilities2KHR surfaceCapabilities{
        .sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR};
    vkGetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice,
        &surfaceInfo,
        &surfaceCapabilities);

    uint32_t surfaceCount = 0;
    vkGetPhysicalDeviceSurfaceFormats2KHR(physicalDevice,
        &surfaceInfo,
        &surfaceCount,
        nullptr);
    std::vector<VkSurfaceFormat2KHR> surfaceFormats(surfaceCount);
    for (size_t i = 0; i < surfaceCount; i++) {
        surfaceFormats[i].sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
    }
    vkGetPhysicalDeviceSurfaceFormats2KHR(physicalDevice,
        &surfaceInfo,
        &surfaceCount,
        surfaceFormats.data());
    VkSurfaceFormat2KHR selectedFormat{
        .sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR};
    for (const auto& availableFormat : surfaceFormats) {
        if (availableFormat.surfaceFormat.format
                == VkFormat::VK_FORMAT_B8G8R8A8_SRGB
            && availableFormat.surfaceFormat.colorSpace
                   == VkColorSpaceKHR::VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            selectedFormat = availableFormat;
        }
    }

    uint32_t presentModesCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice,
        surface,
        &presentModesCount,
        nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModesCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice,
        surface,
        &presentModesCount,
        presentModes.data());
    VkPresentModeKHR presentMode;
    if (std::find(presentModes.begin(),
            presentModes.end(),
            VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR)
        != presentModes.end()) {
        // Tripple buffering
        presentMode = VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR;
    } else {
        // V-Sync
        presentMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .flags = VkSwapchainCreateFlagsKHR{},
        .surface = surface,
        .minImageCount = MAX_FRAMES_IN_SWAPCHAIN,
        .imageFormat = selectedFormat.surfaceFormat.format,
        .imageColorSpace = selectedFormat.surfaceFormat.colorSpace,
        .imageExtent = surfaceCapabilities.surfaceCapabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE,
        .preTransform =
            surfaceCapabilities.surfaceCapabilities.currentTransform,
        .compositeAlpha =
            VkCompositeAlphaFlagBitsKHR::VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = true,
        .oldSwapchain = nullptr};
    MTY_CHECK(vkCreateSwapchainKHR(device,
        &swapchainCreateInfo,
        nullptr,
        &swapchain));
    uint32_t swapchainImageCounts;
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCounts, nullptr);
    std::vector<VkImage> swapChainImages(swapchainImageCounts);
    vkGetSwapchainImagesKHR(device,
        swapchain,
        &swapchainImageCounts,
        swapChainImages.data());

    VkImageSubresourceRange mipMapSubResourceRange{
        .aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1};
    swapchainImageViews.clear();
    VkImageViewCreateInfo imageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = selectedFormat.surfaceFormat.format,
        .components =
            {
                .r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY,
            },

        .subresourceRange = mipMapSubResourceRange,
    };

    for (auto image : swapChainImages) {
        imageViewCreateInfo.image = image;
        VkImageView imageView;
        vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView);
        swapchainImageViews.push_back(imageView);
    }
    std::cout << "";
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
    queueFamilyIndex = -1;
    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if (queueFamilyProperties[i].queueFamilyProperties.queueFlags
                & VK_QUEUE_GRAPHICS_BIT
            && queueFamilyProperties[i].queueFamilyProperties.queueFlags
                   & VK_QUEUE_COMPUTE_BIT) {
            queueFamilyIndex = i;
            break;
        }
    }
    const float queuePriorities[] = {1.0};

    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = queueFamilyIndex,
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
        .enabledExtensionCount = std::size(LOGICAL_DEVICE_EXTENSIONS),
        .ppEnabledExtensionNames = LOGICAL_DEVICE_EXTENSIONS};

    MTY_CHECK(
        vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));

    VkDeviceQueueInfo2 deviceQueueInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
        .queueFamilyIndex = queueFamilyIndex,
        .queueIndex = 0};
    vkGetDeviceQueue2(device, &deviceQueueInfo, &graphicsQueue);
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
    for (const auto& shaderModule : shaderModules) {
        vkDestroyShaderModule(device, shaderModule, nullptr);
    }
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    for (const auto& imageView : swapchainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    glfwTerminate();
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
}

}  // namespace mty