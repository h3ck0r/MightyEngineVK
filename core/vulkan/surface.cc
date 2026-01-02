#include "surface.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include "window_handle.h"

namespace engine_init {

Surface::Surface(const Instance& instance,
    const WindowHandle& window_handle) {
    CreateSurface(instance.instance(), window_handle.window());
}

void Surface::CreateSurface(const vk::Instance& instance,
    GLFWwindow* window) {
    VkSurfaceKHR _surface;
    VkResult res =
        glfwCreateWindowSurface(instance, window, nullptr, &_surface);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    surface_ = vk::UniqueSurfaceKHR(vk::SurfaceKHR(_surface), {instance});
}

}  // namespace engine_init