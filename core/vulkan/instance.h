
#ifndef VK_INSTANCE_H_
#define VK_INSTANCE_H_

#define MINIMAL_VK_VERSION VK_API_VERSION_1_4

#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace engine_init {
struct InstanceParameters {
    uint32_t api_version;
};
struct Instance {
   public:
    Instance();
    // No copy
    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    vk::Instance instance() const { return instance_.get(); }

   private:
    void CreateVulkanInstance();
    bool CheckVersion();
    bool CheckLayersSupport();
    vk::detail::DynamicLoader dl_;
    InstanceParameters instance_parameters_;
    std::string app_name_ = "MightyEngine Sample";
    std::string engine_name_ = "MightyEngine";
    std::vector<const char*> layers_{"VK_LAYER_KHRONOS_validation"};
    vk::UniqueInstance instance_;
};
}  // namespace engine_init

#endif