#pragma once

#include <QObject>
#include <QTimer>

#include <optional>

#include "QtDependComponents/CanController/CanController.h"
#include "QtDependComponents/IOController/IOController.h"
#include "QtDependComponents/PowerController/PowerController.h"

#include "Motion/MotionThread/MotionCommandQueue.h"

#include "PowerDatabase.h"

namespace edm {
namespace app {

class PowerManager final : public QObject {
    Q_OBJECT
public:
    explicit PowerManager(io::IOController::ptr io_ctrler,
                          power::PowerController::ptr power_ctrler,
                          move::MotionCommandQueue::ptr motion_cmd_queue,
                          uint32_t cycle_ms = 1000, QObject *parent = nullptr);
    ~PowerManager();

    const auto &get_db() const { return *power_db_; }

    // 设置操作, 新的eleparam
    bool set_current_eleparam_index(uint32_t index);
    bool set_current_eleparam(const edm::power::EleParam_dkd_t &eleparam);

    // 转发操作数据库
    inline bool exist_eleparam_index(uint32_t index) const {
        return power_db_->exist_index(index);
    }
    inline bool get_eleparam_by_index(uint32_t index,
                                      power::EleParam_dkd_t &output) const {
        return power_db_->get_eleparam_from_index(index, output);
    }
    inline void show_database_ui(bool show = true) {
        power_db_->setVisible(show);
    }

    // 转发操作设置 machon, poweron, machbit 的接口, 并触发相应signal
    inline void set_highpower_on(bool on) {
        power_ctrler_->set_highpower_on(on);
        emit sig_power_flag_changed();
    }
    inline bool is_highpower_on() const {
        return power_ctrler_->is_highpower_on();
    }

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    inline void set_machbit_on(bool on) {
        power_ctrler_->set_machbit_on(on);
        emit sig_power_flag_changed();
    }
    inline bool is_machbit_on() const { return power_ctrler_->is_machbit_on(); }

    inline void set_power_on(bool on) {
        power_ctrler_->set_power_on(on);
        emit sig_power_flag_changed();
    }
    inline bool is_power_on() const { return power_ctrler_->is_power_on(); }
#endif

    inline const auto &get_current_eleparam() const {
        return power_ctrler_->get_current_param();
    }

signals:
    // 告知PowerPanel發生了改變, 让他显示最新的电参数
    void sig_ele_changed(const edm::power::EleParam_dkd_t &eleparam);

    // 告知可能发生了电源位的变化, 刷新相应的io
    void sig_power_flag_changed();

private:
    void _ele_cycle_timer_slot();

private:
    io::IOController::ptr io_ctrler_;
    power::PowerController::ptr power_ctrler_;
    move::MotionCommandQueue::ptr motion_cmd_queue_;

    PowerDatabase *power_db_;

private:
    QTimer *ele_cycle_timer_;
};

} // namespace app
} // namespace edm
