#include "context.h"

#include <iostream>
#include <memory>

#include "command_pool.h"
#include "descriptor_pool.h"
#include "surface.h"
#include "vulkan/context.h"
#include "vulkan/debug_messenger.h"
#include "vulkan/device.h"
#include "vulkan/instance.h"
#include "vulkan/queue.h"
#include "vulkan/queue_family.h"
#include "vulkan/swapchain.h"
#include "vulkan/window_handle.h"

namespace engine_init {
Context::Context() {
    InitVulkan();
}
void Context::InitVulkan() {
    window_handle = std::make_unique<WindowHandle>();
    instance = std::make_unique<Instance>();
    debug_messenger = std::make_unique<DebugMessenger>(*instance);
    surface = std::make_unique<Surface>(*instance, *window_handle);
    physical_device = std::make_unique<PhysicalDevice>(*instance);
    queue_family =
        std::make_unique<QueueFamily>(*physical_device, *surface);
    device = std::make_unique<Device>(*physical_device, *queue_family);
    queue = std::make_unique<Queue>(*device, *queue_family);
    swapchain =
        std::make_unique<Swapchain>(*device, *surface, *queue_family);
    command_pool = std::make_unique<CommandPool>(*device, *queue_family);
    descriptor_pool = std::make_unique<DescriptorPool>(*device);

    std::cout << "Finished initializing engine.";
}
}  // namespace engine_init