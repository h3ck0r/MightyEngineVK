
#ifndef VK_INIT_CONTEXT_H_
#define VK_INIT_CONTEXT_H_

#include "command_pool.h"
#include "debug_messenger.h"
#include "descriptor_pool.h"
#include "queue.h"
#include "swapchain.h"

namespace engine_init {

struct Context {
    Context();
    // No copy
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    std::unique_ptr<Instance> instance;
    std::unique_ptr<WindowHandle> window_handle;
    std::unique_ptr<DebugMessenger> debug_messenger;
    std::unique_ptr<Surface> surface;
    std::unique_ptr<PhysicalDevice> physical_device;
    std::unique_ptr<QueueFamily> queue_family;
    std::unique_ptr<Device> device;

    std::unique_ptr<Queue> queue;
    std::unique_ptr<Swapchain> swapchain;
    std::unique_ptr<CommandPool> command_pool;
    std::unique_ptr<DescriptorPool> descriptor_pool;
    void InitVulkan();
};
}  // namespace engine_init

#endif