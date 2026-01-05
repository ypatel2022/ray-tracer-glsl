cmake_minimum_required(VERSION 3.28)
project(glfwTestProject LANGUAGES CXX C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# GLFW Configuration
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/glfw)

# GLAD Configuration
add_library(glad STATIC
    libs/glad/src/gl.c
    libs/glad/include/glad/gl.h
)
target_include_directories(glad PUBLIC libs/glad/include)

# ImGui
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui)

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(imgui PUBLIC
    glfw
    OpenGL::GL
)

# Define executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/shader.cpp
    src/application.cpp
    include/application.h
    include/utils.h
    include/gl_debug.h
    include/shader.h
)

# Project configuration
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Links everything together
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE
    glad
    imgui
)

# Linux Requirement: Need 'dl' for dynamic loading of GL functions
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
endif()

# Runtime output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# VS Startup Project (Windows)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# Shaders
set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)

file(GLOB SHADER_FILES
    ${SHADER_SOURCE_DIR}/*.vert
    ${SHADER_SOURCE_DIR}/*.frag
)

add_custom_target(copy_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SHADER_FILES}
            ${SHADER_OUTPUT_DIR}
)

add_dependencies(${PROJECT_NAME} copy_shaders)
