#ifndef CORE_ENGINE_
#define CORE_ENGINE_

#include <GLFW/glfw3.h>

#include <vulkan/vulkan_raii.hpp>

namespace core {

class Engine {
   public:
    Engine();
    void run();

   private:
    void loop();
    void cleanup();
    bool initWindow();
    bool initVK();
    bool createVKInstance();
    bool setupDebugMessenger();
    bool pickPhysicalDevice();
    bool createLogicalDevice();
    uint32_t findQueueFamilies();

    vk::raii::Device logicalDevice_;
    vk::raii::Queue deviceQueue_;
    vk::raii::PhysicalDevice physicalDevice_;
    vk::raii::DebugUtilsMessengerEXT debugMessenger_;
    vk::raii::Instance instance_;

    vk::raii::Context context_;
    GLFWwindow* window_;
};

}  // namespace core
#endif  // CORE_ENGINE_