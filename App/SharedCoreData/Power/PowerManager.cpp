#include "PowerManager.h"

#include "Exception/exception.h"
#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {

PowerManager::PowerManager(io::IOController::ptr io_ctrler, power::PowerController::ptr power_ctrler, uint32_t cycle_ms, QObject *parent) : QObject(parent)
  , io_ctrler_(io_ctrler), power_ctrler_(power_ctrler)
{
    power_db_ = new PowerDatabase(); // no parent
    power_db_->hide();

    ele_cycle_timer_ = new QTimer(this);
    connect(ele_cycle_timer_, &QTimer::timeout, this, &PowerManager::_ele_cycle_timer_slot);
    ele_cycle_timer_->start(cycle_ms);
}

PowerManager::~PowerManager()
{
    power_db_->hide();
    delete power_db_;
}

bool PowerManager::set_current_eleparam_index(uint32_t index)
{
    power::EleParam_dkd_t param;
    if (!power_db_->get_eleparam_from_index(index, param)) {
        s_logger->error("set_current_eleparam_index failed, index not exist: {}", index);
        return false;
    }

    return set_current_eleparam(param);
}

bool PowerManager::set_current_eleparam(const power::EleParam_dkd_t &eleparam)
{
    power_ctrler_->update_eleparam_and_send(eleparam);

    emit sig_ele_changed(eleparam);

    return true;
}

void PowerManager::_ele_cycle_timer_slot()
{
    power_ctrler_->trigger_send_eleparam();
    power_ctrler_->trigger_send_ioboard_eleparam();
    io_ctrler_->trigger_send_current_io();
}

} // namespace app
} // namespace edm
