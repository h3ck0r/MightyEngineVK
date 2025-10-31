cmake_minimum_required(VERSION 3.10)

project(VKEngine)

find_package(Vulkan REQUIRED)

add_subdirectory(external/glfw)
add_subdirectory(external/glm)
add_subdirectory(external/vk-bootstrap)


add_executable(VulkanEngineExecutable main.cpp)

target_include_directories(VulkanEngineExecutable PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/external/VulkanMemoryAllocator/include
)

target_link_libraries(VulkanEngineExecutable PUBLIC 
Vulkan::Vulkan 
glfw
vk-bootstrap
glm)

set_target_properties(VulkanEngineExecutable PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)