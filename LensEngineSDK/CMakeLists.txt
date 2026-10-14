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

# Include директории (для компиляции исходников)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# nvision-cpp (feature detection - header-only)
include_directories(${CMAKE_SOURCE_DIR}/../nvision-cpp/libnvision/include)

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
    # Сначала ищем OpenCV 4.8.0 в /usr/local
    set(OpenCV_DIR "/usr/local/lib/cmake/opencv4")
    find_package(OpenCV QUIET HINTS /usr/local)
    if(OpenCV_FOUND)
        message(STATUS "OpenCV found: ${OpenCV_VERSION}")
        include_directories(${OpenCV_INCLUDE_DIRS})
        set(LENSENGINE_USE_OPENCV ON)
        add_definitions(-DLENSENGINE_USE_OPENCV)
    else()
        message(WARNING "OpenCV not found, feature detection will be limited")
        set(LENSENGINE_USE_OPENCV OFF)
    endif()
endif()

# Bullet Physics
find_package(Bullet QUIET)
if(Bullet_FOUND)
    message(STATUS "Bullet Physics found: ${Bullet_VERSION}")
    add_definitions(-DUSE_BULLET_PHYSICS)
    include_directories(/usr/include/bullet)
else()
    message(WARNING "Bullet Physics not found, physics will use fallback")
endif()

# Eigen3 - для DLOTracker
find_package(Eigen3 QUIET)
if(Eigen3_FOUND)
    message(STATUS "Eigen3 found: ${Eigen3_VERSION}")
else()
    message(WARNING "Eigen3 not found, trying to find in common paths")
    find_path(EIGEN3_INCLUDE_DIR Eigen/Dense
        PATHS
        /usr/include
        /usr/local/include
        /usr/include/eigen3
    )
    if(EIGEN3_INCLUDE_DIR)
        include_directories(${EIGEN3_INCLUDE_DIR})
        message(STATUS "Found Eigen3 at: ${EIGEN3_INCLUDE_DIR}")
    else()
        message(WARNING "Eigen3 not found. Please install: sudo apt-get install libeigen3-dev")
    endif()
endif()

# PCL - для DLOTracker
# VTK/PCL fix: создаём MPI::MPI_C imported target для VTK
if(NOT TARGET MPI::MPI_C)
    add_library(MPI::MPI_C SHARED IMPORTED)
    set_target_properties(MPI::MPI_C PROPERTIES
        IMPORTED_LOCATION "/usr/lib/x86_64-linux-gnu/openmpi/lib/libmpi.so"
        INTERFACE_INCLUDE_DIRECTORIES "/usr/lib/x86_64-linux-gnu/openmpi/include"
    )
    add_library(MPI::MPI_CXX SHARED IMPORTED)
    set_target_properties(MPI::MPI_CXX PROPERTIES
        IMPORTED_LOCATION "/usr/lib/x86_64-linux-gnu/libmpi_cxx.so"
        INTERFACE_INCLUDE_DIRECTORIES "/usr/lib/x86_64-linux-gnu/openmpi/include"
        INTERFACE_LINK_LIBRARIES "MPI::MPI_C"
    )
endif()
find_package(PCL QUIET COMPONENTS common filters registration segmentation surface)

# Filter out VTK and Qt5 from PCL_LIBRARIES to avoid Qt5/Qt6 conflict
if(PCL_FOUND)
    set(PCL_LIBS_FILTERED "")
    foreach(lib ${PCL_LIBRARIES})
        if(NOT "${lib}" MATCHES "vtk|VTK")
            list(APPEND PCL_LIBS_FILTERED ${lib})
        endif()
    endforeach()
    set(PCL_LIBRARIES ${PCL_LIBS_FILTERED})
endif()
if(PCL_FOUND)
    message(STATUS "PCL found: ${PCL_VERSION}")
    include_directories(${PCL_INCLUDE_DIRS})
else()
    message(WARNING "PCL not found. Please install: sudo apt-get install libpcl-dev")
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
    src/SLAMTracker.cpp
    src/VisualOdometryTracker.cpp
    src/KalmanFilter6DOF.cpp
    src/IMUDiagnostics.cpp
    src/TemporalFilter.cpp
    src/BundleAdjustment.cpp
    src/DLOTracker.cpp
    src/DLIOWrapper.cpp
    src/NVisionFeatureDetector.cpp
    src/SHLightEstimator.cpp
    src/PlaneDetector.cpp
    src/MeshGenerator.cpp
    src/ShadowRenderer.cpp
    src/HandTracker.cpp
    src/ObjectDetector.cpp
    src/HandGestures.cpp
    src/PhysicsEngine.cpp
    src/SceneUnderstanding.cpp
    src/TestSensorGenerator.cpp
    src/TestRunner.cpp
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
    include/SLAMTracker.h
    include/KalmanFilter6DOF.h
    include/IMUDiagnostics.h
    include/TemporalFilter.h
    include/BundleAdjustment.h
    include/DLOTracker.h
    include/DLIOWrapper.h
    include/NVisionFeatureDetector.h
    include/SHLightEstimator.h
    include/PlaneDetector.h
    include/MeshGenerator.h
    include/ShadowRenderer.h
    include/HandTracker.h
    include/ObjectDetector.h
    include/HandGestures.h
    include/PhysicsEngine.h
    include/SceneUnderstanding.h
    include/TestSensorGenerator.h
    include/TestRunner.h
)

# Создание библиотеки
add_library(LensEngineSDK STATIC
    ${LENSENGINE_SOURCES}
    ${LENSENGINE_HEADERS}
)

# Include директории для библиотеки
target_include_directories(LensEngineSDK PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
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
    target_compile_definitions(LensEngineSDK PRIVATE LENSENGINE_USE_OPENCV)
    message(STATUS "OpenCV DNN enabled for Hand Tracking")
endif()

# Link PCL for DLOTracker
if(PCL_FOUND)
    target_link_libraries(LensEngineSDK
        PRIVATE
        ${PCL_LIBRARIES}
    )
endif()

# Link Eigen3 for DLOTracker
if(Eigen3_FOUND)
    target_link_libraries(LensEngineSDK
        PRIVATE
        Eigen3::Eigen
    )
endif()

# Link Bullet Physics
if(Bullet_FOUND)
    target_link_libraries(LensEngineSDK
        PRIVATE
        ${BULLET_LIBRARIES}
    )
    message(STATUS "Bullet Physics linked")
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

# Тестовое приложение для проверки данных сенсоров
option(BUILD_TESTS "Build test applications" ON)
if(BUILD_TESTS)
    add_executable(test_sensor_data tests/test_sensor_data.cpp)
    target_link_libraries(test_sensor_data LensEngineSDK)
endif()

