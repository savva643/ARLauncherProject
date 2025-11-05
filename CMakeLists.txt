cmake_minimum_required(VERSION 3.15)
project(ARLauncherProject VERSION 1.0.0 LANGUAGES CXX)

# C++ стандарт
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Опции сборки
option(BUILD_SENSOR_CONNECTOR "Build SensorConnector library" ON)
option(BUILD_LENS_ENGINE_SDK "Build LensEngineSDK library" ON)
option(BUILD_AR_LAUNCHER "Build ARLauncher application" ON)
option(USE_OPENGL "Use OpenGL renderer" ON)

# Включаем подпроекты
if(BUILD_LENS_ENGINE_SDK)
    add_subdirectory(LensEngineSDK)
endif()

if(BUILD_AR_LAUNCHER)
    add_subdirectory(ARLauncher)
endif()

# Примечание: SensorConnector собирается через qmake
# (Qt проекты проще собирать через qmake)
message(STATUS "")
message(STATUS "========================================")
message(STATUS "ARLauncherProject Build Configuration")
message(STATUS "========================================")
message(STATUS "SensorConnector: Build with qmake (see SensorConnector/SensorConnector.pro)")
message(STATUS "LensEngineSDK: ${BUILD_LENS_ENGINE_SDK}")
message(STATUS "ARLauncher: ${BUILD_AR_LAUNCHER}")
message(STATUS "Use OpenGL: ${USE_OPENGL}")
message(STATUS "")
message(STATUS "Build order:")
message(STATUS "1. cd SensorConnector && qmake && make")
message(STATUS "2. cd LensEngineSDK/build && cmake .. && make")
message(STATUS "3. cd ARLauncher/build && cmake .. -DUSE_OPENGL=ON && make")
message(STATUS "========================================")

