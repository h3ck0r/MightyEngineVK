#ifndef VK_INIT_CONTEXT_H_
#define VK_INIT_CONTEXT_H_

#include <functional>

#include "../vulkan/command_pool.h"
#include "../vulkan/debug_messenger.h"
#include "../vulkan/descriptor_pool.h"
#include "../vulkan/queue.h"
#include "../vulkan/swapchain.h"
#include "vulkan/physical_device.h"
#include "vulkan/vulkan.hpp"

namespace engine {

struct Context {
   public:
    Context();
    // No copy
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    void InitVulkan();
    void OneTimeSubmit(const std::function<void(vk::CommandBuffer)>& func) const;

    vk::UniqueDescriptorSet AllocateDescSet(vk::DescriptorSetLayout desc_set_layout,
        const vk::DescriptorPool desc_pool,
        vk::Device device);

    engine_lib::WindowHandle& window_handle() const { return *window_handle_.get(); }
    engine_lib::Instance& instance_handle() const { return *instance_.get(); }
    engine_lib::DebugMessenger& debug_messenger_handle() const {
        return *debug_messenger_.get();
    }
    engine_lib::Surface& surface_handle() const { return *surface_.get(); }
    vk::PhysicalDevice physical_device() const {
        return physical_device_.get()->physical_device();
    }
    engine_lib::QueueFamily& queue_family_handle() const { return *queue_family_.get(); }
    vk::Device device() const { return device_.get()->device(); }
    vk::Queue queue() const { return queue_.get()->queue(); }
    engine_lib::Swapchain& swapchain_handle() const { return *swapchain_.get(); }
    engine_lib::CommandPool& command_pool_handle() const { return *command_pool_.get(); }
    engine_lib::DescriptorPool& descriptor_pool_handle() const {
        return *descriptor_pool_.get();
    }

   private:
    // Keep the order of destruction
    std::unique_ptr<engine_lib::WindowHandle> window_handle_;
    std::unique_ptr<engine_lib::Instance> instance_;
    std::unique_ptr<engine_lib::DebugMessenger> debug_messenger_;
    std::unique_ptr<engine_lib::Surface> surface_;
    std::unique_ptr<engine_lib::PhysicalDevice> physical_device_;
    std::unique_ptr<engine_lib::QueueFamily> queue_family_;
    std::unique_ptr<engine_lib::Device> device_;
    std::unique_ptr<engine_lib::Queue> queue_;
    std::unique_ptr<engine_lib::Swapchain> swapchain_;
    std::unique_ptr<engine_lib::CommandPool> command_pool_;
    std::unique_ptr<engine_lib::DescriptorPool> descriptor_pool_;
};
}  // namespace engine

#endif