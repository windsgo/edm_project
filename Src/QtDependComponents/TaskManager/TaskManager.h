#pragma once

#include <QObject>
#include <QMetaObject>
#include <QTimer>

#include <chrono>
#include <memory>
#include <mutex>

namespace edm {

namespace task
{

class InfoHandler : public QObject {
    Q_OBJECT 
    // need to use qtimer to fetch info
public:
    using ptr = std::shared_ptr<InfoHandler>;

public:
    // TODO 外部获取缓存info, 可以无锁, InfoHandler目前和TaskManager, GUI设计在同一个线程

signals: // 提供给TaskManager的信号, 告知一些重要状态变化
    // TODO

private:
    QTimer* info_get_timer_ {nullptr}; // 获取INFO状态, 并处理状态差别, 发出信号给
    // TODO 缓存info (也作为上一次info的记录)
};

class TaskManager final : public QObject {
    Q_OBJECT 
    // need to emit signals ...
public:
    TaskManager() {}
    ~TaskManager() {}

public: // GUI命令接口
    bool operation_start_pointmove();
    bool operation_stop_pointmove();

    bool operation_gcode_reset_list(); // 设置GCode列表
    bool operation_gcode_start();
    bool operation_gcode_pause();
    bool operation_gcode_resume();
    bool operation_gcode_stop();



    // 运行一次Task状态机
    // 包括: 
    // 1. 从info缓存直接刷新所有本地状态
    // 2. G代码状态机维护, 处理本地状态, 发送新的命令给Motion, 并将最新状态通知给上层GUI
    void run_once();


signals: // 提供给GUI界面的信号, 告知一些重要状态变化
    // 通知信号
    void sig_auto_started();
    void sig_auto_paused();
    void sig_auto_resumed();
    void sig_auto_stopped();

    void sig_manual_pointmove_started();
    void sig_manual_pointmove_stopped();

    // 持续状态信号 (只有坐标刷新需要用), 也可以让GUI自己轮训
    void sig_update_axis(); // TODO 参数定义, 信号参数类型注册

private:
    InfoHandler::ptr info_handler_ {nullptr}; // 用于高频周期性获取info状态, 并有信号传回
};

} // namespace task

} // namespace edm