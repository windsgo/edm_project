#pragma once

#include <memory>
#include <random>

#include "Logger/LogDefine.h"
#include "Utils/DataQueueRecorder/DataQueueRecorder.h"
#include "Utils/Filters/SlidingCounter/SlidingCounter.h"
#include "Utils/Filters/SlidingFilter/SlidingFilter.h"
#include "Utils/Format/edm_format.h"
#include "Utils/UnitConverter/UnitConverter.h"

#include "Coordinate/CoordinateSystem.h"
#include "Exception/exception.h"
#include "GlobalCommandQueue/GlobalCommandQueue.h"
#include "Logger/LogMacro.h"
#include "SystemSettings/SystemSettings.h"

#include "QtDependComponents/CanController/CanController.h"
#include "QtDependComponents/IOController/IOController.h"
#include "QtDependComponents/InfoDispatcher/InfoDispatcher.h"
#include "QtDependComponents/PowerController/PowerController.h"
#include "QtDependComponents/ZynqConnection/ZynqConnectController.h"

#include "Interpreter/rs274pyInterpreter/RS274InterpreterWrapper.h"

#include "Motion/MotionSignalQueue/MotionSignalQueue.h"
#include "Motion/MotionThread/MotionThreadController.h"

#include "CanReceiveBuffer/CanReceiveBuffer.h"

#include "HandboxConverter/HandboxConverter.h"

//! above are in libedm.so
//! below are in app

#include "InputHelper/InputHelper.h"

#include "SharedCoreData/Power/PowerManager.h"

#include "Motion/MotionSharedData/MotionSharedData.h"

#include "Logger/LogMacro.h"

#include <QEvent>
#include <QObject>

namespace edm {

namespace app {

class SharedCoreData final : public QObject {
    Q_OBJECT
public:
    SharedCoreData(QObject *parent = nullptr);
    ~SharedCoreData();

public:
    inline const auto& get_system_settings() const { return sys_settings_; }

    inline auto get_coord_system() const { return coord_system_; }
    inline auto get_global_cmd_queue() const { return global_cmd_queue_; }
    inline auto get_can_ctrler() const { return can_ctrler_; }
    inline auto get_io_ctrler() const { return io_ctrler_; }
    inline auto get_power_ctrler() const { return power_ctrler_; }

    inline auto get_motion_signal_queue() const { return motion_signal_queue_; }
    inline auto get_motion_cmd_queue() const { return motion_cmd_queue_; }
    inline auto get_motion_thread_ctrler() const { return motion_thread_ctrler_; }

    inline auto get_can_recv_buffer() const { return can_recv_buffer_; }

    inline auto get_info_dispatcher() const { return info_dispatcher_; }

    inline auto get_power_manager() const { return power_manager_; }

// slots
    // io板蜂鸣器响一下协议
    void send_ioboard_bz_once() const;

    auto get_loglist_logger() const { return loglist_logger_; }
    void set_loglist_logger(log::logger_ptr logger) { loglist_logger_ = logger; }

signals:
    void sig_handbox_start_pointmove(const move::axis_t &dir,
                                     uint32_t speed_level,
                                     bool touch_detect_enable);
    void sig_handbox_stop_pointmove();
    void sig_handbox_pump(bool pump_pn);
    void sig_handbox_ent_auto();
    void sig_handbox_pause_auto();
    void sig_handbox_stop_auto();
    void sig_handbox_ack();

signals: // 发送信号, 连接到主窗口的status bar, 还可以连接到主窗口未来可以放置的信息log窗口
    void sig_info_message(const QString& str, int timeout = 0);
    void sig_warn_message(const QString& str, int timeout = 0);
    void sig_error_message(const QString& str, int timeout = 0);

protected:
    void customEvent(QEvent *e) override;

private:
    void _init_data();
    void _init_handbox_converter(uint32_t can_index);
    void _init_motionthread_cb();


private:
    SystemSettings &sys_settings_ = SystemSettings::instance();

    int can_device_index_ = -1;

    coord::CoordinateSystem::ptr coord_system_;
    global::GlobalCommandQueue::ptr global_cmd_queue_;
    can::CanController::ptr can_ctrler_;
    io::IOController::ptr io_ctrler_;
    power::PowerController::ptr power_ctrler_;

    CanReceiveBuffer::ptr can_recv_buffer_;
    HandboxConverter::ptr handbox_converter_;

    move::MotionSignalQueue::ptr motion_signal_queue_;
    move::MotionCommandQueue::ptr motion_cmd_queue_;
    move::MotionThreadController::ptr motion_thread_ctrler_;

    InfoDispatcher *info_dispatcher_;

    PowerManager* power_manager_;

    zynq::ZynqConnectController::ptr zynq_connect_ctrler_;

private:
    std::function<void(bool)> cb_enable_votalge_gate_;
    std::function<void(bool)> cb_mach_on_;

private:
    log::logger_ptr loglist_logger_ {nullptr};

private:
#ifdef EDM_OFFLINE_RUN_MANUAL_TWO_CAN_DEVICE
    std::thread helper_can_simulate_thread_;
    std::atomic_bool helper_can_simulate_thread_stop_flag_ {false};
#endif // EDM_OFFLINE_RUN_MANUAL_TWO_CAN_DEVICE

public:
    util::DataQueueRecorder<move::MotionSharedData::RecordData1>::ptr record_data1_queuerecorder_;
};

} // namespace app

} // namespace edm
