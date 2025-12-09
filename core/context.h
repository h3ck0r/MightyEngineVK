#ifndef CONTEXT_
#define CONTEXT_

#include <Windows.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>

#include <cstdint>
#include <vector>

#include "GLFW/glfw3.h"

#define WINDOW_WIDTH            1920
#define WINDOW_HEIGHT           1080
#define MAX_FRAMES_IN_SWAPCHAIN 3

// mty == Mighty
namespace mty {
inline constexpr const char* INSTANCE_EXTENSIONS[] = {
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
};
inline constexpr const char* INSTANCE_LAYERS[] = {
    "VK_LAYER_KHRONOS_validation"};
inline constexpr const VkValidationFeatureEnableEXT VALIDATION_EXTENSIONS[] = {
    VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
    VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
    VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
};
inline constexpr const char* LOGICAL_DEVICE_EXTENSIONS[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
};
inline constexpr const char* PHYSICAL_DEVICE_FEATURES[] = {

};

enum class MtyBufferType {
    Scratch,
    AccelInput,
    AccelStorage,
    ShaderBindingTable
};
struct MtyBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDescriptorBufferInfo descBufferInfo;
    uint64_t deviceAddress = 0;
};

struct MtyContext {
    // core func
    void run();
    void initWindow();
    void createInstance();
    void createDeviceAndQueue();
    void createSurfaceAndSwapchain();
    void createCommandPoolAndDescriptorPool();
    void createRayTraycingPipeline();
    void loadFunctions();
    void createDescriptorLayoutAndSet();
    void createRaytracingBindingTable();
    void createSemaphoreAndFence();
    void loop();
    void render();
    void cleanup();

    // utils
    MtyBuffer createBuffer(MtyBufferType type,
        VkDeviceSize,
        const void* data = nullptr);
    VkDescriptorSet allocateDescSet(VkDescriptorSetLayout descriptorSetLayout);

    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    uint32_t queueFamilyIndex;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkCommandPool commandPool;
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    VkSemaphore semaphore;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkShaderModule> shaderModules;

    GLFWwindow* window;

    uint32_t imageIndex = 0;

    // runtime linkage functions
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR_ptr =
        nullptr;
};
}  // namespace mty

#endif  // CONTEXT_