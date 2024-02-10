#include "ServoDevice.h"


namespace edm {

namespace ecat {

PanasonicServoDevice::PanasonicServoDevice(Panasonic_A5B_Ctrl *ctrl,
                                           Panasonic_A5B_Stat *stat) noexcept
    : ctrl_(ctrl), stat_(stat) {}

void PanasonicServoDevice::set_control_word(uint16_t control_word) {
    ctrl_->control_word = control_word;
}

void PanasonicServoDevice::set_target_position(int32_t target_posistion) {
    ctrl_->target_position = target_posistion;
}

void PanasonicServoDevice::set_operation_mode(uint8_t operation_mode) {
    ctrl_->modes_of_operation = operation_mode;
}

uint16_t PanasonicServoDevice::get_status_word() { return stat_->status_word; }

int32_t PanasonicServoDevice::get_actual_position() {
    return stat_->position_actual_value;
}

bool PanasonicServoDevice::sw_fault() const {
    return (stat_->status_word & 0b01001111) == SW_Fault;
}

bool PanasonicServoDevice::sw_switch_on_disabled() const {
    return (stat_->status_word & 0b01001111) == SW_SwitchOnDisabled;
}

bool PanasonicServoDevice::sw_ready_to_switch_on() const {
    return (stat_->status_word & 0b01101111) == SW_ReadyToSwitchOn;
}

bool PanasonicServoDevice::sw_switched_on() const {
    return (stat_->status_word & 0b01101111) == SW_SwitchedOn;
}

bool PanasonicServoDevice::sw_operational_enabled() const {
    return (stat_->status_word & 0b01101111) == SW_OperationEnabled;
}

void PanasonicServoDevice::cw_fault_reset() { ctrl_->control_word = CW_FaultReset; }

void PanasonicServoDevice::cw_shut_down() { ctrl_->control_word = CW_Shutdown; }

void PanasonicServoDevice::cw_switch_on() { ctrl_->control_word = CW_SwitchOn; }

void PanasonicServoDevice::cw_enable_operation() { ctrl_->control_word = CW_EnableOperation; }

void PanasonicServoDevice::sync_actual_position_to_target_position() {
    ctrl_->target_position = stat_->position_actual_value;
}

} // namespace ecat

} // namespace edm