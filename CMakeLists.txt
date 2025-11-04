cmake_minimum_required(VERSION 3.10)

project(VKEngine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

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
