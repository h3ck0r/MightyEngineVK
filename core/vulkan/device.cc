#include "device.h"

#include <vulkan/vulkan_core.h>

#include <iostream>

#include "queue_family.h"

namespace engine_init {

Device::Device(const PhysicalDevice& physical_device,
    const QueueFamily& queue_family) {
    CreateDevice(physical_device.physical_device(),
        queue_family.queue_family_index());
}

void Device::CreateDevice(const vk::PhysicalDevice& physical_device,
    const size_t& queue_family_index) {
    const float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo queue_create_info;
    queue_create_info.setQueueFamilyIndex(queue_family_index);
    queue_create_info.setQueuePriorities(queuePriority);

    const std::vector device_extensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
        VK_KHR_MAINTENANCE3_EXTENSION_NAME,
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    };

    if (!CheckDeviceExtensionSupport(physical_device, device_extensions)) {
        throw std::runtime_error(
            "Some required extensions are not supported");
    }

    vk::DeviceCreateInfo device_info;
    device_info.setQueueCreateInfos(queue_create_info);
    device_info.setPEnabledExtensionNames(device_extensions);

    vk::PhysicalDeviceBufferDeviceAddressFeatures
        buffer_device_address_features{true};
    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR
        ray_tracing_pipeline_features{true};
    vk::PhysicalDeviceAccelerationStructureFeaturesKHR
        acceleration_structure_features{true};
    vk::StructureChain createInfoChain{
        device_info,
        buffer_device_address_features,
        ray_tracing_pipeline_features,
        acceleration_structure_features,
    };

    device_ = physical_device.createDeviceUnique(
        createInfoChain.get<vk::DeviceCreateInfo>());
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*device_);
}

bool Device::CheckDeviceExtensionSupport(
    const vk::PhysicalDevice& physical_device,
    const std::vector<const char*>& required_extensions) const {
    std::vector<vk::ExtensionProperties> available_extensions =
        physical_device.enumerateDeviceExtensionProperties();
    std::vector<std::string> required_extension_names(
        required_extensions.begin(),
        required_extensions.end());

    for (const auto& extension : available_extensions) {
        required_extension_names.erase(
            std::remove(required_extension_names.begin(),
                required_extension_names.end(),
                extension.extensionName),
            required_extension_names.end());
    }

    if (required_extension_names.empty()) {
        std::cout << "All required extensions are supported by "
                     "the device."
                  << std::endl;
        return true;
    } else {
        std::cout << "The following required extensions are not "
                     "supported "
                     "by the device:"
                  << std::endl;
        for (const auto& name : required_extension_names) {
            std::cout << "\t" << name << std::endl;
        }
        return false;
    }
}

}  // namespace engine_init