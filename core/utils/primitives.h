#ifndef VK_PRIMITIVES_H_
#define VK_PRIMITIVES_H_

namespace engine {

struct Vertex {
    float position[3];
};

struct Face {
    float diffuse[3];
    float emission[3];
};

}  // namespace engine
#endif