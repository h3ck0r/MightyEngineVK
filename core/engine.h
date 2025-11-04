#ifndef CORE_ENGINE_
#define CORE_ENGINE_

#include <GLFW/glfw3.h>

#include <vulkan/vulkan_raii.hpp>

namespace core {

class MightyEngine {
   public:
    void run();
    MightyEngine();

   private:
    void loop();
    void cleanup();
    bool initWindow();
    bool initVK();
    bool createSurface();
    bool createVKInstance();
    bool setupDebugMessenger();
    bool pickPhysicalDevice();
    bool createLogicalDevice();
    uint32_t findQueueFamilies();

    vk::raii::Device logicalDevice_ = nullptr;
    vk::raii::Queue deviceQueue_ = nullptr;
    vk::raii::PhysicalDevice physicalDevice_ = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger_ = nullptr;
    vk::raii::Instance instance_ = nullptr;
    vk::raii::SurfaceKHR surface_ = nullptr;

    vk::raii::Context context_;
    GLFWwindow* window_;
};

}  // namespace core
#endif  // CORE_ENGINE_