set(Qt5_DIR $ENV{HOME}/Qt5.14.2/5.14.2/gcc_64/lib/cmake/Qt5)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 COMPONENTS Core Widgets Network SerialBus REQUIRED)
message(STATUS ${Qt5_DIR})
message(STATUS ${Qt5_VERSION})

add_executable(test_pm test_pm.cpp)
add_dependencies(test_pm edm)
target_link_libraries(test_pm edm Qt5::Core Qt5::SerialBus)


add_executable(test_trajectory test_trajectory.cpp)
add_dependencies(test_trajectory edm)
target_link_libraries(test_trajectory edm)

add_executable(testmotiongui testmotiongui.cpp)
add_dependencies(testmotiongui edm)
target_link_libraries(testmotiongui edm Qt5::Core Qt5::SerialBus Qt5::Widgets)
