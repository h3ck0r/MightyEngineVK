#include "core/renderer/renderer.h"

// creates storage for dynamic linking
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

int main() {
    engine::Renderer renderer;
    renderer.MainLoop();
}