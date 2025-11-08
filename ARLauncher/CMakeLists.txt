cmake_minimum_required(VERSION 3.15)
project(ARLauncher VERSION 1.0.0 LANGUAGES CXX)

# C++ стандарт
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Опции сборки
option(USE_VULKAN "Use Vulkan renderer" OFF)
option(USE_OPENGL "Use OpenGL renderer" ON)

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
    add_subdirectory(${LENSENGINE_SDK_DIR} ${CMAKE_BINARY_DIR}/LensEngineSDK)
    set(LENSENGINE_LIB LensEngineSDK)
else()
    message(WARNING "LensEngineSDK not found, building without it")
    set(LENSENGINE_LIB "")
endif()

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

# SensorConnector (опционально, если нужна прямая интеграция)
# Примечание: SensorConnector собирается через qmake и создает libSensorConnector.a
# Для использования SensorConnector в ARLauncher нужно:
# 1. Собрать SensorConnector через qmake
# 2. Подключить Qt библиотеки
# 3. Подключить libSensorConnector.a
# 4. Добавить include директорию SensorConnector/include

# Опция для подключения SensorConnector
# ВНИМАНИЕ: SensorConnector должен быть собран с той же версией Qt, что и ARLauncher
# Сейчас SensorConnector собран с Qt5, а ARLauncher использует Qt6
# Для подключения нужно пересобрать SensorConnector с Qt6
option(USE_SENSOR_CONNECTOR "Use SensorConnector library" OFF)
if(USE_SENSOR_CONNECTOR)
    # Указываем путь к Qt6
    set(Qt6_DIR "/home/savva/Qt/6.5.3/gcc_64/lib/cmake/Qt6" CACHE PATH "Qt6 directory")
    find_package(Qt6 COMPONENTS Core Network Gui REQUIRED)
    set(SENSOR_CONNECTOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../SensorConnector")
    
    if(EXISTS "${SENSOR_CONNECTOR_DIR}/lib/libSensorConnector.a")
        target_link_libraries(ARLauncher PRIVATE 
            ${SENSOR_CONNECTOR_DIR}/lib/libSensorConnector.a
            Qt6::Core
            Qt6::Network
            Qt6::Gui
            turbojpeg
            jpeg
            avcodec
            avformat
            avutil
            swscale
            swresample
        )
        target_include_directories(ARLauncher PRIVATE ${SENSOR_CONNECTOR_DIR}/include)
        target_compile_definitions(ARLauncher PRIVATE USE_SENSOR_CONNECTOR)
        message(STATUS "SensorConnector library linked")
    else()
        message(WARNING "SensorConnector library not found. Build it first with qmake")
    endif()
endif()

# Копируем шейдеры в директорию сборки
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})

# Выходные директории
set_target_properties(ARLauncher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

