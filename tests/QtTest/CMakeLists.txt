set(Qt5_DIR $ENV{HOME}/Qt5.14.2/5.14.2/gcc_64/lib/cmake/Qt5)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 COMPONENTS Core Widgets Network SerialBus REQUIRED)
message(STATUS ${Qt5_DIR})
message(STATUS ${Qt5_VERSION})

add_executable(test_qt test_qt.cpp)
add_dependencies(test_qt edm)
target_link_libraries(test_qt edm Qt5::Core Qt5::SerialBus)


add_executable(test_can test_can.cpp)
add_dependencies(test_can edm)
target_link_libraries(test_can edm Qt5::Core Qt5::SerialBus)

add_executable(test_io test_io.cpp)
add_dependencies(test_io edm)
target_link_libraries(test_io edm Qt5::Core Qt5::SerialBus)