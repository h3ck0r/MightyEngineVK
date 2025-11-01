#ifndef WINDOW_
#define WINDOW_

#include <GLFW/glfw3.h>
#include <memory>

namespace core {

class Window {
   public:
    GLFWwindow* glfw_window;

    void destroy_window();
    void create_surface_glfw();
};

}  // namespace core

#endif  // WINDOW_