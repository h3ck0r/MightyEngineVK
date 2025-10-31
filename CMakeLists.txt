cmake_minimum_required(VERSION 3.10)

project(VKEngine)

find_package(Vulkan REQUIRED)

add_executable(VulkanEngineExecutable main.cpp)

target_link_libraries(VulkanEngineExecutable PUBLIC Vulkan::Vulkan)