#include "PowerManager.h"

#include "Exception/exception.h"
#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {

PowerManager::PowerManager(io::IOController::ptr io_ctrler,
                           power::PowerController::ptr power_ctrler,
                           uint32_t cycle_ms, QObject *parent)
    : QObject(parent), io_ctrler_(io_ctrler), power_ctrler_(power_ctrler) {
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

bool PowerManager::set_current_eleparam(const power::EleParam_dkd_t &eleparam) {
    power_ctrler_->update_eleparam_and_send(eleparam);

    // TODO 可以在这里对motion thread 更新 新的抬刀参数 (up, dn, js, ...)
    //!!!!!!!!!!!!!!!!!!!!!!!!!

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
