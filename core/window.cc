#include "window.h"
#define GGLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <vulkan/vulkan_core.h>
#include <string>

#include <glfw3.h>
#include "globals.h"

namespace core {

Window::Window() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    glfw_window =
        glfwCreateWindow(kWidth, kHeight, kAppName.c_str(), nullptr, nullptr);
}

void Window::create_surface_glfw() {
    VkSurfaceKHR surface = VK_NULL_HANDLE;

}

void Window::destroy_window() {
    glfwDestroyWindow(glfw_window);
    glfwTerminate();
}

}  // namespace core