#ifndef GLOBALS_
#define GLOBALS_

#include <cstdint>
#include <vulkan/vulkan_raii.hpp>

namespace core {
inline constexpr int MAX_FRAMES_IN_FLIGHT = 3;
inline constexpr uint32_t kWindowWidth = 1920;
inline constexpr uint32_t kWindowHeight = 1080;
inline constexpr std::string_view kAppName = "MightyEngine";
const std::vector<char const*> kValidationLayers = {
    "VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> kDeviceExtensions = {
    vk::KHRSwapchainExtensionName,
    vk::KHRDedicatedAllocationExtensionName,
    vk::KHRAccelerationStructureExtensionName,
    vk::KHRDeferredHostOperationsExtensionName,
    vk::KHRRayTracingPipelineExtensionName,
    vk::KHRDedicatedAllocationExtensionName,
    vk::KHRGetMemoryRequirements2ExtensionName,
    vk::KHRMaintenance3ExtensionName,
    vk::KHRPipelineLibraryExtensionName};
// Everything that is mutable during graphics pipeline. Viewport and scissor are
// mutable because we can resize window.
const std::vector kDynamicStates = {vk::DynamicState::eViewport,
    vk::DynamicState::eScissor};
}  // namespace core

#endif  // GLOBALS_