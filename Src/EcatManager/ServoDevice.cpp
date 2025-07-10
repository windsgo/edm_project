#include "ServoDevice.h"
#include "EcatManager/ServoDefines.h"

#ifdef EDM_ECAT_DRIVER_IGH
#include "ecrt.h" // EC_READ_XX, EC_WRITE_XX
#endif // EDM_ECAT_DRIVER_IGH

namespace edm {

namespace ecat {

#ifdef EDM_ECAT_DRIVER_SOEM
PanasonicServoDevice::PanasonicServoDevice(Panasonic_A5B_Ctrl *ctrl,
                                           Panasonic_A5B_Stat *stat) noexcept
    : ctrl_(ctrl), stat_(stat) {}
#endif // EDM_ECAT_DRIVER_SOEM

#ifdef EDM_ECAT_DRIVER_IGH
PanasonicServoDevice::PanasonicServoDevice(
    uint8_t *domain_pd, const Panasonic_A6B_InputDomainOffsets &input_offsets,
    const Panasonic_A6B_OutputDomainOffsets &output_offsets) noexcept
    : domain_pd_(domain_pd), input_offsets_(input_offsets),
      output_offsets_(output_offsets) {}
#endif // EDM_ECAT_DRIVER_IGH

void PanasonicServoDevice::set_control_word(uint16_t control_word) {
#ifdef EDM_ECAT_DRIVER_SOEM
    ctrl_->control_word = control_word;
#endif // EDM_ECAT_DRIVER_SOEM

#ifdef EDM_ECAT_DRIVER_IGH
    EC_WRITE_U16(domain_pd_ + output_offsets_.off_control_word, control_word);
#endif // EDM_ECAT_DRIVER_IGH
}

void PanasonicServoDevice::set_target_position(int32_t target_posistion) {
#ifdef EDM_ECAT_DRIVER_SOEM
    ctrl_->target_position = target_posistion;
#endif // EDM_ECAT_DRIVER_SOEM

#ifdef EDM_ECAT_DRIVER_IGH
    EC_WRITE_S32(domain_pd_ + output_offsets_.off_target_position, target_posistion);
#endif // EDM_ECAT_DRIVER_IGH
}

void PanasonicServoDevice::set_operation_mode(uint8_t operation_mode) {
#ifdef EDM_ECAT_DRIVER_SOEM
    ctrl_->modes_of_operation = operation_mode;
#endif // EDM_ECAT_DRIVER_SOEM

#ifdef EDM_ECAT_DRIVER_IGH
    EC_WRITE_U8(domain_pd_ + output_offsets_.off_modes_of_operation, operation_mode);
#endif // EDM_ECAT_DRIVER_IGH
}

// uint16_t PanasonicServoDevice::get_current_control_word() const {
// #ifdef EDM_ECAT_DRIVER_SOEM
//     return ctrl_->control_word;
// #endif // EDM_ECAT_DRIVER_SOEM

// #ifdef EDM_ECAT_DRIVER_IGH
// #endif // EDM_ECAT_DRIVER_IGH
// }

uint16_t PanasonicServoDevice::get_status_word() const {
#ifdef EDM_ECAT_DRIVER_SOEM
    return stat_->status_word;
#endif // EDM_ECAT_DRIVER_SOEM

#ifdef EDM_ECAT_DRIVER_IGH
    return EC_READ_U16(domain_pd_ + input_offsets_.off_status_word);
#endif // EDM_ECAT_DRIVER_IGH
}

int32_t PanasonicServoDevice::get_actual_position() const {
#ifdef EDM_ECAT_DRIVER_SOEM
    return stat_->position_actual_value;
#endif // EDM_ECAT_DRIVER_SOEM

#ifdef EDM_ECAT_DRIVER_IGH
    return EC_READ_S32(domain_pd_ + input_offsets_.off_position_actual_value);
#endif // EDM_ECAT_DRIVER_IGH
}

int32_t PanasonicServoDevice::get_following_error() const {
#ifdef EDM_ECAT_DRIVER_SOEM
    return stat_->following_error_actual_value;
#endif // EDM_ECAT_DRIVER_SOEM

#ifdef EDM_ECAT_DRIVER_IGH
    return EC_READ_S32(domain_pd_ + input_offsets_.off_following_error_actual_value);
#endif // EDM_ECAT_DRIVER_IGH
}

bool PanasonicServoDevice::sw_fault() const {
    return (get_status_word() & 0b01001111) == SW_Fault;
}

bool PanasonicServoDevice::sw_switch_on_disabled() const {
    return (get_status_word() & 0b01001111) == SW_SwitchOnDisabled;
}

bool PanasonicServoDevice::sw_ready_to_switch_on() const {
    return (get_status_word() & 0b01101111) == SW_ReadyToSwitchOn;
}

bool PanasonicServoDevice::sw_switched_on() const {
    return (get_status_word() & 0b01101111) == SW_SwitchedOn;
}

bool PanasonicServoDevice::sw_operational_enabled() const {
    return (get_status_word() & 0b01101111) == SW_OperationEnabled;
}

void PanasonicServoDevice::cw_fault_reset() {
    set_control_word(CW_FaultReset);
}

void PanasonicServoDevice::cw_shut_down() { set_control_word(CW_Shutdown); }

void PanasonicServoDevice::cw_switch_on() { set_control_word(CW_SwitchOn); }

void PanasonicServoDevice::cw_enable_operation() {
    set_control_word(CW_EnableOperation);
}

void PanasonicServoDevice::cw_disable_operation() {
    set_control_word(CW_DisableOperation);
}

void PanasonicServoDevice::cw_disable_voltage() {
    set_control_word(CW_DisableVoltage);
}

void PanasonicServoDevice::sync_actual_position_to_target_position() {
    set_target_position(get_actual_position());
}

PanasonicServoDeviceWithVOffset::PanasonicServoDeviceWithVOffset(
    uint8_t *domain_pd, const Panasonic_A6B_InputDomainOffsets &input_offsets,
    const Panasonic_A6B_OutputDomainOffsets &output_offsets) noexcept
    : PanasonicServoDevice(domain_pd, input_offsets, output_offsets) {}

void PanasonicServoDeviceWithVOffset::set_v_offset(int32_t v_offset) {
    EC_WRITE_S32(domain_pd_ + output_offsets_.off_v_offset, v_offset);
}

} // namespace ecat

} // namespace edm