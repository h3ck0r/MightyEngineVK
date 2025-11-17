#ifndef CORE_ENGINE_
#define CORE_ENGINE_

#include <GLFW/glfw3.h>

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace core {

class MightyEngine {
   public:
    void run();
    MightyEngine();

    void loop();
    void cleanup();
    void initWindow();

    void initVK();
    void createVKInstance();

    void createSurface();
    void setupDebugMessenger();
    void createLogicalDevice();

    void createSwapChain();
    void recreateSwapChain();
    void createImageViews();
    void cleanupSwapChain();

    void drawFrame();
    void createGraphicsPipeline();
    void recordCommandBuffer(uint32_t imageIndex);

    void createVertexBuffer();
    uint32_t findMemoryType(uint32_t typeFilter,
        vk::MemoryPropertyFlags properties);

    void transitioImageLayout(uint32_t imageIndex,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::AccessFlags2 srcAccessMask,
        vk::AccessFlags2 dstAccessMask,
        vk::PipelineStageFlags2 srcStageMask,
        vk::PipelineStageFlags2 dstStageMask);
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(
        const std::vector<char>& code) const;

    vk::raii::Device device = nullptr;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::raii::Instance instance = nullptr;
    vk::raii::SurfaceKHR surface = nullptr;
    vk::raii::Context context;

    vk::raii::Queue deviceQueue = nullptr;
    vk::raii::Queue presentQueue = nullptr;
    uint32_t graphicsQueueFamilyIndex = 0;

    vk::raii::SwapchainKHR swapChain = nullptr;
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::raii::ImageView> swapChainImageViews;
    vk::Extent2D swapChainExtent;
    vk::SurfaceFormatKHR swapChainSurfaceFormat;
    vk::raii::PipelineLayout graphicsPipelineLayout = nullptr;
    vk::raii::Pipeline graphicsPipeline = nullptr;

    vk::raii::CommandPool commandPool = nullptr;
    std::vector<vk::raii::CommandBuffer> commandBuffers;

    vk::raii::Buffer vertexBuffer = nullptr;
    vk::raii::DeviceMemory vertexBufferMemory = nullptr;

    std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;

    uint32_t currentFrame = 0;
    uint32_t semaphoreIndex = 0;
    bool frameBufferResized = false;

    GLFWwindow* window;
};

}  // namespace core
#endif  // CORE_ENGINE_