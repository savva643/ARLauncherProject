# Install script for directory: /home/savva/Documents/ARLauncherProject/LensEngineSDK

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/savva/Documents/ARLauncherProject/LensEngineSDK/build2/lib/libLensEngineSDK.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/LensEngineSDK" TYPE FILE FILES
    "/home/savva/Documents/ARLauncherProject/LensEngineSDK/include/LensEngine.h"
    "/home/savva/Documents/ARLauncherProject/LensEngineSDK/include/LensEngineTypes.h"
    "/home/savva/Documents/ARLauncherProject/LensEngineSDK/include/LensEngineAPI.h"
    "/home/savva/Documents/ARLauncherProject/LensEngineSDK/include/SensorFusionEKF.h"
    "/home/savva/Documents/ARLauncherProject/LensEngineSDK/include/Lidar3DProcessor.h"
    "/home/savva/Documents/ARLauncherProject/LensEngineSDK/include/SpatialMappingSystem.h"
    "/home/savva/Documents/ARLauncherProject/LensEngineSDK/include/ARDataProcessor.h"
    "/home/savva/Documents/ARLauncherProject/LensEngineSDK/include/CameraController.h"
    "/home/savva/Documents/ARLauncherProject/LensEngineSDK/include/SLAMTracker.h"
    "/home/savva/Documents/ARLauncherProject/LensEngineSDK/include/KalmanFilter6DOF.h"
    "/home/savva/Documents/ARLauncherProject/LensEngineSDK/include/IMUDiagnostics.h"
    "/home/savva/Documents/ARLauncherProject/LensEngineSDK/include/TemporalFilter.h"
    "/home/savva/Documents/ARLauncherProject/LensEngineSDK/include/BundleAdjustment.h"
    "/home/savva/Documents/ARLauncherProject/LensEngineSDK/include/DLOTracker.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/savva/Documents/ARLauncherProject/LensEngineSDK/build2/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
