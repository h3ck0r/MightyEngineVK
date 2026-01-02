
#ifndef VK_INIT_DEBUG_MESSENGER_H_
#define VK_INIT_DEBUG_MESSENGER_H_

#include <vulkan/vulkan.hpp>

#include "instance.h"

namespace engine_init {

struct DebugMessenger {
    DebugMessenger(const Instance& instance);
    // No copy
    DebugMessenger(const DebugMessenger&) = delete;
    DebugMessenger& operator=(const DebugMessenger&) = delete;

   private:
    void CreateDebugMessenger(const vk::Instance& instance);
    vk::UniqueDebugUtilsMessengerEXT messenger_;
};
}  // namespace engine_init

#endif