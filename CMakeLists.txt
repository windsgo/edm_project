cmake_minimum_required(VERSION 3.14)

project(edm LANGUAGES C CXX VERSION 0.0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE is set default to Release")
else()
    message(STATUS "CMAKE_BUILD_TYPE is set to ${CMAKE_BUILD_TYPE}")
endif()

# set(CMAKE_CXX_COMPILER /usr/bin/g++-13)

## config
add_definitions(-DEDM_ROOT_DIR="${PROJECT_SOURCE_DIR}/") ## 最后要有斜杠

## libs
# fmt library
set(fmt_DIR ${PROJECT_SOURCE_DIR}/third_party/fmt/lib/cmake/fmt)
find_package(fmt REQUIRED)

# json library
include_directories(third_party/meojson/include)

# python3
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# boost python
find_package(Boost COMPONENTS python REQUIRED)

# spdlog
# list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
# include(spdlog)
# set(SPDLOG_BUILD_PIC ON CACHE BOOL)
add_subdirectory(third_party/spdlog)
target_compile_options(spdlog PRIVATE -fPIC)

# soem
add_subdirectory(third_party/SOEM-1.4.0)
target_compile_options(soem PRIVATE -fPIC -Wno-error=stringop-truncation -Wno-stringop-truncation)
set(SOEM_INCLUDE_DIRS third_party/SOEM-1.4.0/soem/ third_party/SOEM-1.4.0/osal/ third_party/SOEM-1.4.0/osal/linux/ third_party/SOEM-1.4.0/oshw/linux/)

# find qt5
set(Qt5_DIR $ENV{HOME}/Qt5.14.2/5.14.2/gcc_64/lib/cmake/Qt5)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 COMPONENTS Core SerialBus Widgets REQUIRED)

## Project
set(PROJECT_SOURCES
    Src/Interpreter/rs274pyInterpreter/RS274InterpreterWrapper.cpp
    Src/Logger/LogManager.cpp
    Src/Logger/LogDefine.cpp
    Src/EcatManager/ServoDevice.cpp
    Src/EcatManager/EcatManager.cpp
    Src/QtDependComponents/CanController/CanController.cpp
    Src/QtDependComponents/IOController/IOController.cpp
    Src/QtDependComponents/PowerController/PowerController.cpp
    Src/QtDependComponents/PowerController/EleparamDecoder.cpp
    Src/Utils/Netif/netif_utils.cpp
    Src/Utils/UnitConverter/UnitConverter.cpp
    Src/Utils/DataQueueRecorder/DataQueueRecorder.cpp
    Src/Motion/Moveruntime/Moveruntime.cpp
    Src/Motion/MoveruntimeWrapper/MoveruntimeWrapper.cpp
    Src/Motion/PointMoveHandler/PointMoveHandler.cpp
    Src/Motion/MotionThread/MotionCommandQueue.cpp 
    Src/Motion/MotionThread/MotionThreadController.cpp
    Src/Motion/MotionSignalQueue/MotionSignalQueue.cpp
    Src/Motion/MotionStateMachine/MotionStateMachine.cpp
    Src/Motion/MotionStateMachine/MotionAutoTask.cpp
    Src/Motion/MotionStateMachine/G00AutoTask.cpp
    Src/Motion/MotionStateMachine/G01AutoTask.cpp
    Src/Motion/MotionStateMachine/MotionAutoTaskRunner.cpp
    Src/Motion/PauseMoveController/AxisRecorder.cpp
    Src/Motion/PauseMoveController/PauseMoveController.cpp
    Src/Motion/SignalBuffer/SignalBuffer.cpp
    Src/Motion/TouchDetectHandler/TouchDetectHandler.cpp
    Src/Motion/Trajectory/TrajectorySegement.cpp
    Src/Motion/Trajectory/TrajectoryList.cpp
    Src/GlobalCommandQueue/GlobalCommandQueue.cpp
    Src/QtDependComponents/InfoDispatcher/InfoDispatcher.cpp
    Src/Coordinate/Coordinate.cpp
    Src/Coordinate/CoordinateManager.cpp
    Src/Coordinate/CoordinateSystem.cpp
    Src/SystemSettings/SystemSettings.cpp
    Src/CanReceiveBuffer/CanReceiveBuffer.cpp
    Src/HandboxConverter/HandboxConverter.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/Src)
include_directories(${SOEM_INCLUDE_DIRS})

add_library(edm SHARED ${PROJECT_SOURCES})
target_compile_options(edm PUBLIC -fPIC -Wno-error=stringop-overflow -Wno-stringop-overflow) # gcc bug with fmtlib
target_link_libraries(edm PUBLIC 
    spdlog::spdlog
    fmt::fmt # fmtlib, use if libstdc++ does not support <format>
    Boost::python 
    Boost::boost
    Python3::Python
    soem
    Qt5::Core Qt5::SerialBus
    m # <math.h> or <cmath>
    pthread # <pthread.h>
)

add_subdirectory(App)
add_subdirectory(tests)
