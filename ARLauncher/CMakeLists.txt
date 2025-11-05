cmake_minimum_required(VERSION 3.15)
project(ARLauncher VERSION 1.0.0 LANGUAGES CXX)

# C++ стандарт
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Опции сборки
option(USE_VULKAN "Use Vulkan renderer" ON)
option(USE_OPENGL "Use OpenGL renderer" OFF)

if(USE_VULKAN)
    find_package(Vulkan QUIET)
    if(NOT Vulkan_FOUND)
        message(WARNING "Vulkan not found, falling back to OpenGL")
        set(USE_VULKAN OFF)
        set(USE_OPENGL ON)
    endif()
endif()

if(USE_OPENGL)
    find_package(OpenGL REQUIRED)
    if(NOT OpenGL_FOUND)
        message(FATAL_ERROR "OpenGL not found! Please install OpenGL development libraries")
    endif()
endif()

# GLFW для создания окна
find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
    message(STATUS "GLFW not found via package, trying to find manually")
    find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h
        PATHS
        /usr/include
        /usr/local/include
        C:/glfw/include
    )
    find_library(GLFW_LIBRARY glfw3
        PATHS
        /usr/lib
        /usr/local/lib
        C:/glfw/lib
    )
    if(GLFW_INCLUDE_DIR AND GLFW_LIBRARY)
        set(GLFW_FOUND TRUE)
        include_directories(${GLFW_INCLUDE_DIR})
        message(STATUS "Found GLFW at: ${GLFW_INCLUDE_DIR}")
    else()
        message(WARNING "GLFW not found! Please install GLFW")
        message(STATUS "You can download GLFW from: https://www.glfw.org/download.html")
        message(STATUS "Or install via vcpkg: vcpkg install glfw3:x64-windows")
    endif()
endif()

# GLM для математики
find_package(glm QUIET)
if(NOT glm_FOUND)
    find_path(GLM_INCLUDE_DIR glm/glm.hpp
        PATHS
        /usr/include
        /usr/local/include
        C:/glm/include
    )
    if(GLM_INCLUDE_DIR)
        include_directories(${GLM_INCLUDE_DIR})
        message(STATUS "Found GLM at: ${GLM_INCLUDE_DIR}")
    else()
        message(WARNING "GLM not found! Please install GLM")
        message(STATUS "You can download GLM from: https://github.com/g-truc/glm/releases")
        message(STATUS "Or install via vcpkg: vcpkg install glm:x64-windows")
    endif()
else()
    message(STATUS "GLM found via package")
endif()

# LensEngineSDK
set(LENSENGINE_SDK_DIR "${CMAKE_SOURCE_DIR}/../LensEngineSDK")
if(EXISTS "${LENSENGINE_SDK_DIR}/CMakeLists.txt")
    add_subdirectory(${LENSENGINE_SDK_DIR} ${CMAKE_BINARY_DIR}/LensEngineSDK)
    set(LENSENGINE_LIB LensEngineSDK)
else()
    message(WARNING "LensEngineSDK not found, building without it")
    set(LENSENGINE_LIB "")
endif()

# Include директории
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/ui)

# Исходные файлы
set(ARLAUNCHER_SOURCES
    src/main.cpp
    src/Application.cpp
    src/Renderer.cpp
    src/Scene.cpp
    src/Camera.cpp
    ui/UIRenderer.cpp
    ui/UIElement.cpp
    ui/Window.cpp
    ui/Widget.cpp
    ui/Button.cpp
    ui/Text.cpp
    ui/Input.cpp
)

set(ARLAUNCHER_HEADERS
    include/Application.h
    include/Renderer.h
    include/Scene.h
    include/Camera.h
    ui/UIRenderer.h
    ui/UIElement.h
    ui/Window.h
    ui/Widget.h
    ui/Button.h
    ui/Text.h
    ui/Input.h
    ui/Style.h
)

# Создание исполняемого файла
add_executable(ARLauncher
    ${ARLAUNCHER_SOURCES}
    ${ARLAUNCHER_HEADERS}
)

# Связывание библиотек
target_link_libraries(ARLauncher
    PRIVATE
)

# GLFW
if(glfw3_FOUND)
    target_link_libraries(ARLauncher PRIVATE glfw)
    target_include_directories(ARLauncher PRIVATE ${glfw3_INCLUDE_DIR})
    message(STATUS "Linking GLFW via package")
elseif(GLFW_FOUND)
    target_link_libraries(ARLauncher PRIVATE ${GLFW_LIBRARY})
    target_include_directories(ARLauncher PRIVATE ${GLFW_INCLUDE_DIR})
    message(STATUS "Linking GLFW manually")
else()
    message(FATAL_ERROR "GLFW not found! Please install GLFW or set GLFW_INCLUDE_DIR and GLFW_LIBRARY")
endif()

if(USE_VULKAN AND Vulkan_FOUND)
    target_link_libraries(ARLauncher PRIVATE Vulkan::Vulkan)
    target_compile_definitions(ARLauncher PRIVATE USE_VULKAN)
elseif(USE_OPENGL)
    target_link_libraries(ARLauncher PRIVATE ${OPENGL_LIBRARIES})
    target_include_directories(ARLauncher PRIVATE ${OPENGL_INCLUDE_DIRS})
    target_compile_definitions(ARLauncher PRIVATE USE_OPENGL)
endif()

if(LENSENGINE_LIB)
    target_link_libraries(ARLauncher PRIVATE ${LENSENGINE_LIB})
    target_include_directories(ARLauncher PRIVATE ${LENSENGINE_SDK_DIR}/include)
endif()

# Выходные директории
set_target_properties(ARLauncher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

