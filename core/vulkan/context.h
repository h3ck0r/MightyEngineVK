
#ifndef VK_INIT_CONTEXT_H_
#define VK_INIT_CONTEXT_H_

#include <functional>

#include "command_pool.h"
#include "debug_messenger.h"
#include "descriptor_pool.h"
#include "queue.h"
#include "swapchain.h"
#include "vulkan/vulkan.hpp"

namespace engine_init {

struct Context {
    Context();
    // No copy
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    void InitVulkan();
    void OneTimeSubmit(const std::function<void(vk::CommandBuffer)>& func) const;
    uint32_t FindMemoryType(uint32_t type_filter,
        vk::MemoryPropertyFlags properties) const;
    vk::UniqueDescriptorSet AllocateDescSet(vk::DescriptorSetLayout desc_set_layout,
        const vk::DescriptorPool desc_pool,
        vk::Device device);

    WindowHandle& window_handle() const { return *window_handle_.get(); }
    Instance& instance_handle() const { return *instance_.get(); }
    DebugMessenger& debug_messenger_handle() const { return *debug_messenger_.get(); }
    Surface& surface_handle() const { return *surface_.get(); }
    PhysicalDevice& physical_device_handle() const { return *physical_device_.get(); }
    QueueFamily& queue_family_handle() const { return *queue_family_.get(); }
    vk::Device device() const { return device_.get()->device(); }
    vk::Queue queue() const { return queue_.get()->queue(); }
    Swapchain& swapchain_handle() const { return *swapchain_.get(); }
    CommandPool& command_pool_handle() const { return *command_pool_.get(); }
    DescriptorPool& descriptor_pool_handle() const { return *descriptor_pool_.get(); }

    // Keep the order of destruction
    std::unique_ptr<WindowHandle> window_handle_;
    std::unique_ptr<Instance> instance_;
    std::unique_ptr<DebugMessenger> debug_messenger_;
    std::unique_ptr<Surface> surface_;
    std::unique_ptr<PhysicalDevice> physical_device_;
    std::unique_ptr<QueueFamily> queue_family_;
    std::unique_ptr<Device> device_;
    std::unique_ptr<Queue> queue_;
    std::unique_ptr<Swapchain> swapchain_;
    std::unique_ptr<CommandPool> command_pool_;
    std::unique_ptr<DescriptorPool> descriptor_pool_;
};
}  // namespace engine_init

#endif