#include "window_handle.h"

#define STB_IMAGE_IMPLEMENTATION

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include "stb_image.h"

namespace engine_init {

WindowHandle::WindowHandle() {
    CreateWindow();
}

void WindowHandle::CreateWindow() {
    int width, height, channels;
    unsigned char* pixels =
        stbi_load("assets/ICON.png", &width, &height, &channels, 4);

    GLFWimage image;
    image.height = height;
    image.width = width;
    image.pixels = pixels;

    if (!glfwInit()) {
        throw std::runtime_error("failed to init glfw!");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window_ =
        glfwCreateWindow(WIDTH, HEIGHT, "Mighty Engine", nullptr, nullptr);
    glfwSetWindowIcon(window_, 1, &image);
}

}  // namespace engine_init