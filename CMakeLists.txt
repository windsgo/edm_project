cmake_minimum_required(VERSION 3.14)

project(edm LANGUAGES CXX VERSION 0.0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE is set default to Release")
else()
    message(STATUS "CMAKE_BUILD_TYPE is set to ${CMAKE_BUILD_TYPE}")
endif()

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
add_subdirectory(third_party/spdlog)

## Project
set(PROJECT_SOURCES
    Interpreter/rs274pyInterpreter/src/RS274InterpreterWrapper.cpp
    Logger/LogManager.cpp
)
set(PROJECT_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/Interpreter/rs274pyInterpreter/include  
    ${PROJECT_SOURCE_DIR}/Logger
)

add_library(edm SHARED ${PROJECT_SOURCES})
target_include_directories(edm PUBLIC ${PROJECT_INCLUDE_DIRS})
target_link_libraries(edm PUBLIC 
    spdlog::spdlog
    fmt::fmt 
    Boost::python 
    Python3::Python
)

add_subdirectory(tests)
