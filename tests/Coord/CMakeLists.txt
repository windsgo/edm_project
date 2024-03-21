set(Qt5_DIR $ENV{HOME}/Qt5.14.2/5.14.2/gcc_64/lib/cmake/Qt5)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 COMPONENTS Core Widgets Network SerialBus REQUIRED)
message(STATUS ${Qt5_DIR})
message(STATUS ${Qt5_VERSION})

# add_executable(test_coord test_coord.cpp)
# add_dependencies(test_coord edm)
# target_link_libraries(test_coord edm Qt5::Core Qt5::SerialBus)

