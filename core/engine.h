#ifndef CORE_ENGINE_
#define CORE_ENGINE_

#include "VkBootstrap.h"
#include "window.h"

namespace core {

class Engine {
   public:
    void run();

   private:
    void init();
    void loop();
    void cleanup();
    void initVK();
    void createInstance();

    Window window_;
    vkb::Device vkb_device_;
    vkb::Instance vkb_inst_;
};

}  // namespace core
#endif  // CORE_ENGINE_