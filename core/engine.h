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
    bool createSwapChain();
    bool createImageViews();
    bool createGraphicsPipeline();
    bool createCommandPool();
    uint32_t findQueueFamilies();
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(
        const std::vector<char>& code) const;

    vk::raii::Device logicalDevice_ = nullptr;
    vk::raii::PhysicalDevice physicalDevice_ = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger_ = nullptr;
    vk::raii::Instance instance_ = nullptr;
    vk::raii::SurfaceKHR surface_ = nullptr;
    vk::raii::Context context_;

    vk::raii::Queue deviceQueue_ = nullptr;
    vk::raii::Queue presentQueue_ = nullptr;
    uint32_t graphicsIndex_;
    uint32_t presentIndex_;

    vk::raii::SwapchainKHR swapChain_ = nullptr;
    std::vector<vk::Image> swapChainImages_;
    std::vector<vk::raii::ImageView> swapChainImageViews_;
    vk::Extent2D swapChainExtent_;
    vk::SurfaceFormatKHR swapChainSurfaceFormat_;
    vk::raii::PipelineLayout graphicsPipelineLayout_ = nullptr;
    vk::raii::Pipeline graphicsPipeline_ = nullptr;

    vk::raii::CommandPool commandPool_ = nullptr;

    GLFWwindow* window_;
};

}  // namespace core
#endif  // CORE_ENGINE_