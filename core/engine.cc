#include "engine.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include "VkBootstrap.h"
#include "globals.h"
#include "window.h"

namespace core {

void Engine::run() {
    init();
    loop();
    cleanup();
}
void Engine::init() {}
void Engine::loop() {
    while (!glfwWindowShouldClose(window_.glfw_window)) {
        glfwPollEvents();
    }
}
void Engine::initVK() {
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("Example Vulkan Application")
                        .request_validation_layers()
                        .use_default_debug_messenger()
                        .build();
    if (!inst_ret) {
        return;
    }
    vkb_inst_ = inst_ret.value();

    vkb::PhysicalDeviceSelector selector{vkb_inst_};
    auto phys_ret = selector.set_surface(surface)
                        .set_minimum_version(1, 1)
                        .require_dedicated_transfer_queue()
                        .select();
    if (!phys_ret) {
        return;
    }

    vkb::DeviceBuilder device_builder{phys_ret.value()};
    auto dev_ret = device_builder.build();
    if (!dev_ret) {
        return;
    }
    vkb_device_ = dev_ret.value();

    auto graphics_queue_ret = vkb_device_.get_queue(vkb::QueueType::graphics);
    if (!graphics_queue_ret) {
        return;
    }
    VkQueue graphics_queue = graphics_queue_ret.value();
}
void Engine::cleanup() { window_.destroy_window(); }

}  // namespace core