cmake_minimum_required(VERSION 3.15)
project(ARLauncher VERSION 1.0.0 LANGUAGES CXX)

# C++ стандарт
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Опции сборки
# Vulkan — основной рендерер GlaskiOS. OpenGL — LEGACY (не развивается, оставлен только для старых машин).
option(USE_VULKAN "Use Vulkan renderer (main)" ON)
option(USE_OPENGL "LEGACY: OpenGL renderer" OFF)

if(USE_VULKAN)
    set(USE_OPENGL OFF)
    find_package(Vulkan QUIET)
    if(NOT Vulkan_FOUND)
        message(FATAL_ERROR "Vulkan not found: sudo apt install libvulkan-dev vulkan-tools glslang-tools "
                            "(или -DUSE_VULKAN=OFF -DUSE_OPENGL=ON для LEGACY OpenGL)")
    endif()
endif()

if(USE_OPENGL)
    find_package(OpenGL REQUIRED)
    if(NOT OpenGL_FOUND)
        message(FATAL_ERROR "OpenGL not found! Please install OpenGL development libraries")
    endif()
endif()

# FreeType для рендеринга текста
find_package(Freetype QUIET)
if(NOT Freetype_FOUND)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(FREETYPE freetype2)
    endif()
    if(NOT FREETYPE_FOUND)
        find_path(FREETYPE_INCLUDE_DIRS freetype/freetype.h
            PATHS
            /usr/include
            /usr/local/include
            /usr/include/freetype2
            /usr/local/include/freetype2
            C:/freetype/include
        )
        find_library(FREETYPE_LIBRARIES freetype
            PATHS
            /usr/lib
            /usr/local/lib
            C:/freetype/lib
        )
        if(FREETYPE_INCLUDE_DIRS AND FREETYPE_LIBRARIES)
            set(FREETYPE_FOUND TRUE)
            message(STATUS "Found FreeType at: ${FREETYPE_INCLUDE_DIRS}")
        else()
            message(WARNING "FreeType not found! Text rendering will be limited")
            message(STATUS "Install FreeType: sudo apt-get install libfreetype6-dev (Linux)")
            message(STATUS "Or: vcpkg install freetype:x64-windows (Windows)")
        endif()
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
set(LENSENGINE_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../LensEngineSDK")
if(EXISTS "${LENSENGINE_SDK_DIR}/CMakeLists.txt")
    set(CMAKE_DISABLE_FIND_PACKAGE_Qt5 TRUE)
    add_subdirectory(${LENSENGINE_SDK_DIR} ${CMAKE_BINARY_DIR}/LensEngineSDK)
    unset(CMAKE_DISABLE_FIND_PACKAGE_Qt5)
    set(LENSENGINE_LIB LensEngineSDK)
else()
    message(WARNING "LensEngineSDK not found, building without it")
    set(LENSENGINE_LIB "")
endif()

# SensorStream — приём потока iPhone (протокол v2), без Qt
set(SENSORSTREAM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../SensorStream")
set(SENSORSTREAM_BUILD_TOOLS ON CACHE BOOL "Build stream_simulator / stream_probe")
add_subdirectory(${SENSORSTREAM_DIR} ${CMAKE_BINARY_DIR}/SensorStream)

# Include директории (используем текущий source dir, чтобы корректно работать как под-проект)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ui)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# Исходные файлы
set(ARLAUNCHER_SOURCES
    src/main.cpp
    src/Application.cpp
    src/Renderer.cpp
    src/Scene.cpp
    src/Camera.cpp
    src/ARLauncherAPI.cpp
    src/FontRenderer.cpp
    src/LiDARVisualizer.cpp
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
    include/ARLauncherAPI.h
    include/FontRenderer.h
    include/LiDARVisualizer.h
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

# FreeType
if(Freetype_FOUND)
    target_link_libraries(ARLauncher PRIVATE Freetype::Freetype)
    target_compile_definitions(ARLauncher PRIVATE USE_FREETYPE)
    message(STATUS "FreeType linked via package")
elseif(FREETYPE_FOUND)
    target_link_libraries(ARLauncher PRIVATE ${FREETYPE_LIBRARIES})
    target_include_directories(ARLauncher PRIVATE ${FREETYPE_INCLUDE_DIRS})
    target_compile_definitions(ARLauncher PRIVATE USE_FREETYPE)
    message(STATUS "FreeType linked manually")
else()
    message(WARNING "FreeType not found - text rendering will use simple glyphs")
endif()

if(LENSENGINE_LIB)
    target_link_libraries(ARLauncher PRIVATE ${LENSENGINE_LIB})
    target_include_directories(ARLauncher PRIVATE ${LENSENGINE_SDK_DIR}/include)
endif()

# PCL/Eigen for DLOTracker (static lib needs these propagated)
find_package(Eigen3 QUIET)
if(Eigen3_FOUND)
    target_link_libraries(ARLauncher PRIVATE Eigen3::Eigen)
endif()
# Link ONLY core PCL libs (avoid VTK/Qt5 transitive deps)
target_link_libraries(ARLauncher PRIVATE
    pcl_common pcl_filters pcl_registration pcl_kdtree pcl_search pcl_features
)
target_include_directories(ARLauncher PRIVATE /usr/include/pcl-1.14 /usr/include/eigen3)

# GLM include директории
if(GLM_INCLUDE_DIR)
    target_include_directories(ARLauncher PRIVATE ${GLM_INCLUDE_DIR})
elseif(glm_FOUND)
    # GLM найден через find_package
    # GLM обычно header-only, поэтому просто добавляем include директории
    if(TARGET glm::glm)
        target_link_libraries(ARLauncher PRIVATE glm::glm)
    endif()
    # В любом случае добавляем стандартные пути для GLM
    target_include_directories(ARLauncher PRIVATE
        /usr/include
        /usr/local/include
    )
else()
    # Если GLM не найден, пробуем стандартные пути
    target_include_directories(ARLauncher PRIVATE
        /usr/include
        /usr/local/include
    )
endif()

# Поток с iPhone принимает SensorStream (без Qt, в своих потоках).
# Старый Qt SensorConnector (../SensorConnector) — LEGACY, лаунчером больше не используется.
target_link_libraries(ARLauncher PRIVATE SensorStream)

# Копируем шейдеры в директорию сборки
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})

# Vulkan shaders (GLSL -> SPIR-V)
if(USE_VULKAN AND Vulkan_FOUND)
    find_program(GLSLANG_VALIDATOR glslangValidator)
    if(GLSLANG_VALIDATOR)
        file(GLOB VK_SHADER_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders_vk/*.vert
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders_vk/*.frag
        )

        set(VK_SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders_vk)
        file(MAKE_DIRECTORY ${VK_SHADER_OUTPUT_DIR})

        set(VK_SHADER_SPV_OUTPUTS "")
        foreach(SHADER ${VK_SHADER_SOURCES})
            get_filename_component(FILE_NAME ${SHADER} NAME)
            set(SPV ${VK_SHADER_OUTPUT_DIR}/${FILE_NAME}.spv)
            add_custom_command(
                OUTPUT ${SPV}
                COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER} -o ${SPV}
                DEPENDS ${SHADER}
                COMMENT "Compiling Vulkan shader ${FILE_NAME}"
            )
            list(APPEND VK_SHADER_SPV_OUTPUTS ${SPV})
        endforeach()

        add_custom_target(ARLauncherVulkanShaders ALL DEPENDS ${VK_SHADER_SPV_OUTPUTS})
        add_dependencies(ARLauncher ARLauncherVulkanShaders)
    else()
        message(WARNING "glslangValidator not found. Vulkan shaders will not be compiled. Install Vulkan SDK / glslang-tools.")
    endif()
endif()

# Выходные директории
set_target_properties(ARLauncher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

