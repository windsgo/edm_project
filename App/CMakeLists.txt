# find qt5
set(Qt5_DIR $ENV{HOME}/Qt5.14.2/5.14.2/gcc_64/lib/cmake/Qt5)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 COMPONENTS Core SerialBus Widgets Sql REQUIRED)

# qwt
set(QWT_ROOT ${PROJECT_SOURCE_DIR}/third_party/qwt-6.2.0-x86_64)
set(QWT_INCLUDE_DIR ${QWT_ROOT}/include/)
set(QWT_LIBRARY_DIR ${QWT_ROOT}/lib/)
set(QWT_LIBRARIES qwt)
message(STATUS ${QWT_LIBRARY_DIR})

set(APP_SOURCES
    main.cpp
    SharedCoreData/SharedCoreData.cpp
    SharedCoreData/Power/PowerDatabase.cpp
    SharedCoreData/Power/PowerManager.cpp
    CoordPanel/CoordPanel.cpp
    MainWindow/MainWindow.cpp
    InfoPanel/InfoPanel.cpp
    MovePanel/MovePanel.cpp
    IOPanel/IOPanel.cpp
    PowerPanel/PowerPanel.cpp
    TaskManager/TaskManager.cpp  
    TaskManager/GCodeTaskConverter.cpp
    TaskManager/GCodeRunner.cpp
    GCodePanel/GCodePanel.cpp
    TestPanel/TestPanel.cpp
    codeeditor/codeeditor.cpp
    codeeditor/highlighter.cpp
    DataDisplayer/DataDisplayer.cpp
    CoordSettingPanel/CoordSettingPanel.cpp
    SystemSettingPanel/SystemSettingPanel.cpp
    DataQueueRecordPanel/DataQueueRecordPanel.cpp
    Resource/app_resource.qrc # qt resource
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(app ${APP_SOURCES})
add_dependencies(app edm)
target_include_directories(app PUBLIC ${QWT_INCLUDE_DIR})
target_link_directories(app PUBLIC ${QWT_LIBRARY_DIR})
target_link_libraries(app PUBLIC edm Qt5::Core Qt5::Widgets Qt5::SerialBus Qt5::Sql ${QWT_LIBRARIES})

# SharedCoreData 用于创建并共享核心数据(如InfoDispatcher, GlobalCommandQueue, CanController等)
# 并用于构建不同窗体模块的信号槽桥梁 (简化信号槽的传递路径)
