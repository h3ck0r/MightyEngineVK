cmake_minimum_required(VERSION 3.10)
project(VKEngine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(SLANGC_EXECUTABLE "C:/VulkanSDK/1.4.328.1/bin/slangc.exe")
set(SHADER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/core/shaders/shader.slang)
set(SHADERS_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/shaders")

add_custom_target(CompileShaders ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_OUT_DIR}
    COMMAND ${SLANGC_EXECUTABLE} ${SHADER_FILE}
        -target spirv
        -profile spirv_1_4
        -emit-spirv-directly
        -fvk-use-entrypoint-name
        -entry vertMain -entry fragMain
        -o ${SHADERS_OUT_DIR}/slang.spv
    COMMENT "Compiling Slang shader!!!!"
    VERBATIM
)

find_package(Vulkan REQUIRED)
add_subdirectory(external/glfw)
add_subdirectory(external/glm)

file(GLOB CORE_SOURCES "core/*.cc")

add_executable(VulkanEngineExecutable main.cc ${CORE_SOURCES})
target_include_directories(VulkanEngineExecutable PRIVATE ${Vulkan_INCLUDE_DIRS} core)
target_link_libraries(VulkanEngineExecutable PUBLIC Vulkan::Vulkan glfw glm)

add_dependencies(VulkanEngineExecutable CompileShaders)
