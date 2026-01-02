#include "debug_messenger.h"

#include <vulkan/vulkan_core.h>

#include <iostream>

namespace engine_init {

static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugUtilsMessengerCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT message_severity,
    vk::DebugUtilsMessageTypeFlagsEXT message_types,
    vk::DebugUtilsMessengerCallbackDataEXT const* callback_data,
    void* pUserData) {
    std::cerr << callback_data->pMessage << std::endl;
    return VK_FALSE;
};

DebugMessenger::DebugMessenger(const Instance& instance) {
    CreateDebugMessenger(instance.instance());
}

void DebugMessenger::CreateDebugMessenger(const vk::Instance& instance) {
    vk::DebugUtilsMessengerCreateInfoEXT messenger_info;
    messenger_info.setMessageSeverity(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    messenger_info.setMessageType(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    messenger_info.setPfnUserCallback(&DebugUtilsMessengerCallback);
    messenger_ =
        instance.createDebugUtilsMessengerEXTUnique(messenger_info);
}
}  // namespace engine_init