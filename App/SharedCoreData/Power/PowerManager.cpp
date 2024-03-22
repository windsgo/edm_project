#include "PowerManager.h"

#include "Exception/exception.h"
#include "Logger/LogMacro.h"

#include "SystemSettings/SystemSettings.h"
#include "TaskManager/TaskHelper.h"
#include "Utils/UnitConverter/UnitConverter.h"

#include "Motion/MotionThread/MotionCommand.h"
#include "Motion/MotionThread/MotionCommandQueue.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {

PowerManager::PowerManager(io::IOController::ptr io_ctrler,
                           power::PowerController::ptr power_ctrler,
                           move::MotionCommandQueue::ptr motion_cmd_queue,
                           uint32_t cycle_ms, QObject *parent)
    : QObject(parent), io_ctrler_(io_ctrler), power_ctrler_(power_ctrler),
      motion_cmd_queue_(motion_cmd_queue) {
    power_db_ = new PowerDatabase(); // no parent
    power_db_->hide();

    ele_cycle_timer_ = new QTimer(this);
    connect(ele_cycle_timer_, &QTimer::timeout, this,
            &PowerManager::_ele_cycle_timer_slot);
    ele_cycle_timer_->start(cycle_ms);

    // 转发信号
    connect(power_db_, &PowerDatabase::sig_select_param, this,
            &PowerManager::sig_ele_changed);

    // 初始化一个电参数
    auto default_ele = power_db_->get_one_valid_eleparam();
    if (default_ele) {
        s_logger->debug("hi");
        this->set_current_eleparam(*default_ele);
    }

    // test
    power::EleParam_dkd_t p;
    bool ret = power_db_->get_eleparam_from_index(1, p);
    s_logger->debug("hi2: {}", ret);
    auto strs = power::PowerController::eleparam_to_string(p);
    for (const auto &s : strs) {
        s_logger->trace(s);
    }
}

PowerManager::~PowerManager() {
    power_db_->hide();
    delete power_db_;
}

bool PowerManager::set_current_eleparam_index(uint32_t index) {
    power::EleParam_dkd_t param;
    if (!power_db_->get_eleparam_from_index(index, param)) {
        s_logger->error(
            "set_current_eleparam_index failed, index not exist: {}", index);
        return false;
    }

    return set_current_eleparam(param);
}

static inline double _get_speed_blu_s_from_js(uint32_t js) {
    // js = 01 -> 0.1m/min = 100mm/min
    if (js > 99) js = 99;
    return util::UnitConverter::mm_min2blu_s((double)js * 100.0);
}

bool PowerManager::set_current_eleparam(const power::EleParam_dkd_t &eleparam) {
    power_ctrler_->update_eleparam_and_send(eleparam);

    // TODO 可以在这里对motion thread 更新 新的抬刀参数 (up, dn, js, ...)
    //!!!!!!!!!!!!!!!!!!!!!!!!!

    const auto &sys_conf_jp = SystemSettings::instance().get_jump_param();

    move::JumpParam jp;
    jp.up_blu = util::UnitConverter::mm2blu(eleparam.up);
    jp.dn_ms = eleparam.dn * 100; // dn = 10, means 1s=1000ms

    jp.buffer_blu = sys_conf_jp.buffer_um;
    if (jp.buffer_blu < 0.0) jp.buffer_blu = 0.0;

    jp.speed_param.acc0 =
        util::UnitConverter::um2blu(sys_conf_jp.max_acc_um_s2);
    jp.speed_param.dec0 = -jp.speed_param.acc0;
    jp.speed_param.entry_v = 0;
    jp.speed_param.exit_v = 0;
    jp.speed_param.nacc = util::UnitConverter::ms2p(sys_conf_jp.nacc_ms);
    jp.speed_param.cruise_v = _get_speed_blu_s_from_js(eleparam.jump_js);

    auto cmd = std::make_shared<move::MotionCommandSettingSetJumpParam>(jp);
    motion_cmd_queue_->push_command(cmd);
    auto ret = task::TaskHelper::WaitforCmdTobeAccepted(cmd, -1);
    if (!ret) {
        return false;
    }

    emit sig_ele_changed(eleparam);

    return true;
}

void PowerManager::_ele_cycle_timer_slot() {
    power_ctrler_->trigger_send_eleparam();
    power_ctrler_->trigger_send_ioboard_eleparam();
    io_ctrler_->trigger_send_current_io();
}

} // namespace app
} // namespace edm
