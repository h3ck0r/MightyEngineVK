#ifndef CONTEXT_
#define CONTEXT_

#include <Windows.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>

#include "GLFW/glfw3.h"

#define WINDOW_WIDTH  1920
#define WINDOW_HEIGHT 1080

// mty = Mighty
namespace mty {
inline constexpr const char* INSTANCE_EXTENSIONS[] = {
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
inline constexpr const char* INSTANCE_LAYERS[] = {
    "VK_LAYER_KHRONOS_validation"};
inline constexpr const VkValidationFeatureEnableEXT VALIDATION_EXTENSIONS[] = {
    VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
    VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
    VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
};
inline constexpr const char* DEVICE_EXTENSIONS[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

struct MtyContext {
    void run();
    void createInstance();
    void createDeviceAndQueue();
    void createSurfaceAndSwapchain();
    void loop();
    void cleanup();

    VkInstance instance;

    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue queue;
    VkSurfaceKHR surface;
    GLFWwindow* window;
};
}  // namespace mty

#endif  // CONTEXT_