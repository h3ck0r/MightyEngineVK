
#ifndef VK_INIT_SURFACE_H_
#define VK_INIT_SURFACE_H_

#include <vulkan/vulkan.hpp>

#include "instance.h"
#include "vulkan/vulkan.hpp"
#include "window_handle.h"

namespace engine_init {

struct Surface {
   public:
    Surface(const Instance& instance, const WindowHandle& window_handle);
    // No copy
    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;
    vk::SurfaceKHR surface() const { return surface_.get(); }

   private:
    void CreateSurface(const vk::Instance& instance, GLFWwindow* window);
    vk::UniqueSurfaceKHR surface_;
};
}  // namespace engine_init

#endif