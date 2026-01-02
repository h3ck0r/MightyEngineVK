#include "instance.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include "iostream"

namespace engine_init {

Instance::Instance() {
    auto vk_get_instance_proc_addr =
        dl_.getProcAddress<PFN_vkGetInstanceProcAddr>(
            "vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_get_instance_proc_addr);
    if (!CheckVersion()) {
        // TODO: ADD EXCEPTION
        return;
    }
    CreateVulkanInstance();
}

bool Instance::CheckVersion() {
    uint32_t api_version;
    vk::Result result = vk::enumerateInstanceVersion(&api_version);

    if (result != vk::Result::eSuccess) {
        api_version = VK_API_VERSION_1_0;
    }

    if (api_version < MINIMAL_VK_VERSION) {
        std::cout << "Installed Vulkan version is too low for "
                     "this engine. "
                     "Minimum version: "
                  << MINIMAL_VK_VERSION;
        return false;
    }

    uint32_t major = VK_API_VERSION_MAJOR(api_version);
    uint32_t minor = VK_API_VERSION_MINOR(api_version);
    uint32_t patch = VK_API_VERSION_PATCH(api_version);

    std::cout << "Supported Vulkan Version: " << major << "." << minor
              << "." << patch << "\n";

    instance_parameters_.api_version = api_version;
    return true;
}

void Instance::CreateVulkanInstance() {
    // Prepase extensions and layers
    uint32_t glfw_extension_count = 0;
    const char** glfwExtensions =
        glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::vector extensions(glfwExtensions,
        glfwExtensions + glfw_extension_count);
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    extensions.push_back(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);

    std::vector<vk::ValidationFeatureEnableEXT> enabled_features = {
        vk::ValidationFeatureEnableEXT::eBestPractices,
        vk::ValidationFeatureEnableEXT::eSynchronizationValidation};
    vk::ValidationFeaturesEXT validation_features;
    validation_features.setEnabledValidationFeatures(enabled_features);

    vk::ApplicationInfo app_info;
    app_info.setApiVersion(instance_parameters_.api_version);
    app_info.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0));
    app_info.setEngineVersion(VK_MAKE_VERSION(1, 0, 0));
    app_info.setPApplicationName(app_name_.c_str());
    app_info.setPEngineName(engine_name_.c_str());

    CheckLayersSupport();
    vk::InstanceCreateInfo instance_info;
    instance_info.setPApplicationInfo(&app_info);
    instance_info.setPEnabledLayerNames(layers_);
    instance_info.setPEnabledExtensionNames(extensions);
    instance_info.setPNext(&validation_features);
    instance_ = vk::createInstanceUnique(instance_info);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance_);
}

bool Instance::CheckLayersSupport() {
    std::vector<vk::LayerProperties> available_layers =
        vk::enumerateInstanceLayerProperties();

    for (const char* layer_name : layers_) {
        bool layer_found = false;

        for (const auto& layer_properties : available_layers) {
            if (std::strcmp(layer_name, layer_properties.layerName) == 0) {
                layer_found = true;
                break;
            }
        }

        if (!layer_found) {
            std::cerr << "Requested layer not found: " << layer_name
                      << "\n";
            return false;
        }
    }
    return true;
}
}  // namespace engine_init