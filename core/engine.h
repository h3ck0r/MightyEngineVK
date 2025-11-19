#ifndef CORE_ENGINE_
#define CORE_ENGINE_
#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_EXCEPTIONS
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include <GLFW/glfw3.h>

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "utils.h"
#include "vulkan/vulkan.hpp"

namespace core {

struct MightyBuffer {
    enum class Type {
        Scratch,
        AccelerationInput,
        AccelerationStorage,
        ShaderBindingTable,
    };
    MightyBuffer() = default;

    vk::raii::Buffer buffer = nullptr;
    vk::raii::DeviceMemory memory = nullptr;
    vk::DescriptorBufferInfo descriptorBufferInfo;
    uint64_t deviceAddress = 0;
};

struct MightyAccelerationStruct {
    MightyAccelerationStruct() = default;

    MightyBuffer buffer;
    vk::raii::AccelerationStructureKHR accelerationStruct = nullptr;
    vk::WriteDescriptorSetAccelerationStructureKHR descriptorAccelInfo;
};

class MightyEngine {
   public:
    void run();
    MightyEngine();

    void loop();
    void cleanup();

    void initVK();

    void recreateSwapChain();
    void cleanupSwapChain();
    void createImageViews();
    void createSwapChain();

    void drawFrame();
    void recordCommandBuffer(uint32_t imageIndex);
    void updateUniformBuffer(uint32_t currentFrame);

    void copyBuffer(vk::raii::Buffer& srcBuffer,
        vk::raii::Buffer& dstBuffer,
        vk::DeviceSize size);

    MightyBuffer createBuffer(MightyBuffer::Type type,
        vk::DeviceSize size,
        const void* data = nullptr);

    MightyAccelerationStruct createAccelerationStruct(
        vk::AccelerationStructureGeometryKHR geometry,
        uint32_t primitiveCount,
        vk::AccelerationStructureTypeKHR type);

    void transitioImageLayout(uint32_t imageIndex,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::AccessFlags2 srcAccessMask,
        vk::AccessFlags2 dstAccessMask,
        vk::PipelineStageFlags2 srcStageMask,
        vk::PipelineStageFlags2 dstStageMask);
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(
        const std::vector<char>& code) const;

    vk::raii::Device logicalDevice = nullptr;
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

    vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
    vk::raii::PipelineLayout graphicsPipelineLayout = nullptr;
    vk::raii::Pipeline graphicsPipeline = nullptr;

    vk::raii::CommandPool commandPool = nullptr;
    std::vector<vk::raii::CommandBuffer> commandBuffers;

    std::vector<vk::raii::Buffer> uniformBuffers;
    std::vector<vk::raii::DeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    vk::raii::DescriptorPool descriptorPool = nullptr;
    std::vector<vk::raii::DescriptorSet> descriptorSets;

    std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;

    uint32_t currentFrame = 0;
    uint32_t semaphoreIndex = 0;
    bool frameBufferResized = false;

    GLFWwindow* window;

    MightyBuffer vertexBuffer;
    MightyBuffer indexBuffer;
    MightyBuffer materialBuffer;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Material> materials;
};

}  // namespace core
#endif  // CORE_ENGINE_