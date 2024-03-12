# find qt5
set(Qt5_DIR $ENV{HOME}/Qt5.14.2/5.14.2/gcc_64/lib/cmake/Qt5)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 COMPONENTS Core SerialBus Widgets REQUIRED)

set(APP_SOURCES
    main.cpp
    SharedCoreData/SharedCoreData.cpp
    CoordPanel/CoordPanel.cpp
    MainWindow/MainWindow.cpp
    InfoPanel/InfoPanel.cpp
    MovePanel/MovePanel.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(app ${APP_SOURCES})
add_dependencies(app edm)
target_link_libraries(app PUBLIC edm Qt5::Core Qt5::Widgets Qt5::SerialBus)

# SharedCoreData 用于创建并共享核心数据(如InfoDispatcher, GlobalCommandQueue, CanController等)
# 并用于构建不同窗体模块的信号槽桥梁 (简化信号槽的传递路径)
