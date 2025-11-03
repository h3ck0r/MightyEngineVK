cmake_minimum_required(VERSION 3.10)

project(VKEngine)

find_package(Vulkan REQUIRED)

add_subdirectory(external/glfw)
add_subdirectory(external/glm)

file(GLOB CORE_SOURCES "core/*.cc")

add_executable(VulkanEngineExecutable main.cpp 
    ${CORE_SOURCES})

target_include_directories(VulkanEngineExecutable PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    core
)

target_link_libraries(VulkanEngineExecutable PUBLIC 
Vulkan::Vulkan 
glfw
glm)

set_target_properties(VulkanEngineExecutable PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)