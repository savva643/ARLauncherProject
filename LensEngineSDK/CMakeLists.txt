cmake_minimum_required(VERSION 3.15)
project(LensEngineSDK VERSION 1.0.0 LANGUAGES CXX)

# C++ стандарт
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Опции сборки
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(LENSENGINE_USE_OPENCV "Use OpenCV for feature detection" ON)

# Выходные директории
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include директории
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Зависимости
# GLM - математическая библиотека
find_package(glm QUIET)
if(NOT glm_FOUND)
    # Попробуем найти через pkg-config
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(GLM glm QUIET)
    endif()
    
    if(NOT GLM_FOUND)
        message(STATUS "glm not found, trying to find in common paths")
        # Попробуем стандартные пути
        find_path(GLM_INCLUDE_DIR glm/glm.hpp
            PATHS
            /usr/include
            /usr/local/include
            C:/glm/include
            C:/vcpkg/installed/x64-windows/include
        )
        
        if(GLM_INCLUDE_DIR)
            set(GLM_FOUND TRUE)
            include_directories(${GLM_INCLUDE_DIR})
            message(STATUS "Found GLM at: ${GLM_INCLUDE_DIR}")
        else()
            message(WARNING "GLM not found. Please install GLM or add it to CMAKE_PREFIX_PATH")
            message(STATUS "You can download GLM from: https://github.com/g-truc/glm/releases")
            message(STATUS "Or install via vcpkg: vcpkg install glm:x64-windows")
        endif()
    endif()
else()
    message(STATUS "GLM found: ${glm_VERSION}")
endif()

if(LENSENGINE_USE_OPENCV)
    find_package(OpenCV QUIET)
    if(OpenCV_FOUND)
        message(STATUS "OpenCV found: ${OpenCV_VERSION}")
        include_directories(${OpenCV_INCLUDE_DIRS})
    else()
        message(WARNING "OpenCV not found, feature detection will be limited")
        set(LENSENGINE_USE_OPENCV OFF)
    endif()
endif()

# Исходные файлы
set(LENSENGINE_SOURCES
    src/LensEngine.cpp
    src/LensEngineAPI.cpp
    src/SensorFusionEKF.cpp
    src/Lidar3DProcessor.cpp
    src/SpatialMappingSystem.cpp
    src/ARDataProcessor.cpp
    src/CameraController.cpp
)

set(LENSENGINE_HEADERS
    include/LensEngine.h
    include/LensEngineTypes.h
    include/LensEngineAPI.h
    include/SensorFusionEKF.h
    include/Lidar3DProcessor.h
    include/SpatialMappingSystem.h
    include/ARDataProcessor.h
    include/CameraController.h
)

# Создание библиотеки
add_library(LensEngineSDK STATIC
    ${LENSENGINE_SOURCES}
    ${LENSENGINE_HEADERS}
)

# Связывание библиотек
target_link_libraries(LensEngineSDK
    PRIVATE
)

if(LENSENGINE_USE_OPENCV AND OpenCV_FOUND)
    target_link_libraries(LensEngineSDK
        PRIVATE
        ${OpenCV_LIBS}
    )
    target_compile_definitions(LENSENGINE_USE_OPENCV PRIVATE LENSENGINE_USE_OPENCV)
endif()

# Установка
install(TARGETS LensEngineSDK
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${LENSENGINE_HEADERS}
    DESTINATION include/LensEngineSDK
)

# Пример приложения (опционально)
option(BUILD_EXAMPLES "Build example applications" OFF)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

