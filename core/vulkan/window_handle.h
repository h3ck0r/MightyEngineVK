
#ifndef VK_INIT_WINDOW_H_
#define VK_INIT_WINDOW_H_
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#define WIDTH  1920
#define HEIGHT 1080

namespace engine_init {

struct WindowHandle {
   public:
    WindowHandle();
    // No copy
    WindowHandle(const WindowHandle&) = delete;
    WindowHandle& operator=(const WindowHandle&) = delete;
    GLFWwindow* window() const { return window_; }

   private:
    void CreateWindow();
    GLFWwindow* window_;
};
}  // namespace engine_init

#endif