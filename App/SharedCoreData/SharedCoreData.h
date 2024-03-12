#pragma once

#include <memory>

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

// TODO TaskManager

#include "Motion/MotionSignalQueue/MotionSignalQueue.h"
#include "Motion/MotionThread/MotionThreadController.h"

#include "CanReceiveBuffer/CanReceiveBuffer.h"

#include "HandboxConverter/HandboxConverter.h"

#include "InputHelper/InputHelper.h"

#include <QEvent>
#include <QObject>

namespace edm {

namespace app {

class SharedCoreData final : public QObject {
    Q_OBJECT
public:
    SharedCoreData(QObject *parent = nullptr);
    ~SharedCoreData() noexcept = default;

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

protected:
    void customEvent(QEvent *e) override;

private:
    void _init_data();
    void _init_handbox_converter(uint32_t can_index);
    void _init_motionthread_cb();


private:
    SystemSettings &sys_settings_ = SystemSettings::instance();

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


private:
    std::function<bool(void)> cb_get_touch_physical_detected_;
    std::function<double(void)> cb_get_servo_cmd_;
    std::function<void(bool)> cb_enable_votalge_gate_;

#ifdef EDM_OFFLINE_MANUAL_TOUCH_DETECT
    bool manual_touch_detect_flag_{false};
#endif // EDM_OFFLINE_MANUAL_TOUCH_DETECT

#ifdef EDM_OFFLINE_MANUAL_SERVO_CMD
    double manual_servo_cmd_um_{0.0};
#endif // EDM_OFFLINE_MANUAL_SERVO_CMD

public:
#ifdef EDM_OFFLINE_MANUAL_TOUCH_DETECT
    inline void set_manual_touch_detect_flag(bool detected) {
        manual_touch_detect_flag_ = detected;
    }
#endif // EDM_OFFLINE_MANUAL_TOUCH_DETECT

#ifdef EDM_OFFLINE_MANUAL_SERVO_CMD
    inline void set_manual_servo_cmd_um(double cmd_um) {
        manual_servo_cmd_um_ = cmd_um;
    }
#endif // EDM_OFFLINE_MANUAL_SERVO_CMD
};

} // namespace app

} // namespace edm
