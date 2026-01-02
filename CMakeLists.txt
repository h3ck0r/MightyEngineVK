cmake_minimum_required(VERSION 3.10)
project(VKEngine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")

set(COMPILE_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/compile.bat)
set(SHADERS_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/shaders")
set(SHADERS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/core/shaders")

add_definitions(-DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

add_custom_target(CompileShaders ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_OUT_DIR}
    COMMAND ${COMPILE_SCRIPT} "${SHADERS_SRC_DIR}" "${SHADERS_OUT_DIR}"
    COMMENT "Running shader compilation script (compile.bat)!"
    VERBATIM
)

set(ASSETS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(ASSETS_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/assets")

add_custom_target(CopyAssets ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ASSETS_OUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${ASSETS_SRC_DIR} ${ASSETS_OUT_DIR}
    COMMENT "Copying assets to binary folder..."
    VERBATIM
)

find_package(Vulkan REQUIRED)
add_subdirectory(external/glfw)
add_subdirectory(external/glm)
add_subdirectory(external/tinyobjloader)

file(GLOB_RECURSE CORE_SOURCES "core/*.cc")

add_executable(VulkanEngineExecutable main.cc ${CORE_SOURCES})
target_include_directories(VulkanEngineExecutable PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    core
    external/stb
)
target_link_libraries(VulkanEngineExecutable PUBLIC Vulkan::Vulkan glfw glm tinyobjloader)

add_dependencies(VulkanEngineExecutable CompileShaders CopyAssets)

