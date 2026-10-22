cmake_minimum_required(VERSION 3.15)
project(SensorStream VERSION 2.0 LANGUAGES CXX)

# Приёмник потока iPhone (протокол v2) без Qt.
# Заменяет SensorConnector (Qt, legacy) в горячем пути ARLauncher.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SS_FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavutil libswscale)

add_library(SensorStream STATIC
    src/StreamReceiver.cpp
    src/VideoDecoder.cpp
    src/ReturnEncoder.cpp
    include/SensorStream/Protocol.h
    include/SensorStream/Types.h
    include/SensorStream/ClockSync.h
    include/SensorStream/VideoDecoder.h
    include/SensorStream/StreamReceiver.h
    include/SensorStream/ReturnEncoder.h
)
target_include_directories(SensorStream PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(SensorStream PUBLIC PkgConfig::SS_FFMPEG Threads::Threads)
if(NOT CMAKE_BUILD_TYPE)
    target_compile_options(SensorStream PRIVATE -O2)
endif()

option(SENSORSTREAM_BUILD_TOOLS "Build stream_simulator / stream_probe" ON)
if(SENSORSTREAM_BUILD_TOOLS)
    add_executable(stream_probe tools/stream_probe.cpp)
    target_link_libraries(stream_probe PRIVATE SensorStream)

    add_executable(stream_simulator tools/stream_simulator.cpp)
    target_link_libraries(stream_simulator PRIVATE SensorStream PkgConfig::SS_FFMPEG)
endif()
