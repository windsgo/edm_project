#pragma once

#include <cstdint>
#include <memory>

#include "Panasonic.h"
#include "ServoDefines.h"

// #ifdef EDM_ECAT_DRIVER_IGH
// #include "ecrt.h"
// #endif // EDM_ECAT_DRIVER_IGH

namespace edm {

namespace ecat {

class ServoDevice {
public:
    using ptr = std::shared_ptr<ServoDevice>;
    ServoDevice() noexcept = default;
    virtual ~ServoDevice() noexcept = default;

    virtual constexpr ServoType type() const = 0;

    virtual void set_control_word(uint16_t control_word) = 0;
    virtual void set_target_position(int32_t target_posistion) = 0;
    virtual void set_operation_mode(uint8_t operation_mode) = 0;

    // virtual uint16_t get_current_control_word() const = 0;

    virtual uint16_t get_status_word() const = 0;
    virtual int32_t get_actual_position() const = 0;

    virtual int32_t get_following_error() const = 0;

    virtual bool sw_fault() const = 0;
    virtual bool sw_switch_on_disabled() const = 0;
    virtual bool sw_ready_to_switch_on() const = 0;
    virtual bool sw_switched_on() const = 0;
    virtual bool sw_operational_enabled() const = 0;

    virtual void cw_fault_reset() = 0;
    virtual void cw_shut_down() = 0;
    virtual void cw_switch_on() = 0;
    virtual void cw_enable_operation() = 0;

    virtual void cw_disable_operation() = 0;
    virtual void cw_disable_voltage() = 0;

    virtual void sync_actual_position_to_target_position() = 0;
};

class PanasonicServoDevice final : public ServoDevice {
public:
#ifdef EDM_ECAT_DRIVER_SOEM
    PanasonicServoDevice(Panasonic_A5B_Ctrl *ctrl,
                         Panasonic_A5B_Stat *stat) noexcept;
#endif // EDM_ECAT_DRIVER_SOEM
#ifdef EDM_ECAT_DRIVER_IGH
    PanasonicServoDevice(
        uint8_t *domain_pd,
        const Panasonic_A6B_InputDomainOffsets &input_offsets,
        const Panasonic_A6B_OutputDomainOffsets &output_offsets) noexcept;
#endif // EDM_ECAT_DRIVER_IGH
    virtual ~PanasonicServoDevice() noexcept = default;

    PanasonicServoDevice(const PanasonicServoDevice &) = delete;
    PanasonicServoDevice &operator=(const PanasonicServoDevice &) = delete;
    PanasonicServoDevice(PanasonicServoDevice &&) = delete;
    PanasonicServoDevice &operator=(PanasonicServoDevice &&) = delete;

    constexpr ServoType type() const override {
        return ServoType::Panasonic_A5B;
    }

    void set_control_word(uint16_t control_word) override;
    void set_target_position(int32_t target_posistion) override;
    void set_operation_mode(uint8_t operation_mode) override;

    // uint16_t get_current_control_word() const override;

    uint16_t get_status_word() const override;
    int32_t get_actual_position() const override;

    int32_t get_following_error() const override;

    bool sw_fault() const override;
    bool sw_switch_on_disabled() const override;
    bool sw_ready_to_switch_on() const override;
    bool sw_switched_on() const override;
    bool sw_operational_enabled() const override;

    void cw_fault_reset() override;
    void cw_shut_down() override;
    void cw_switch_on() override;
    void cw_enable_operation() override;

    void cw_disable_operation() override;
    void cw_disable_voltage() override;

    void sync_actual_position_to_target_position() override;

private:
#ifdef EDM_ECAT_DRIVER_SOEM
    volatile Panasonic_A5B_Ctrl *ctrl_;
    volatile Panasonic_A5B_Stat *stat_;
#endif // EDM_ECAT_DRIVER_SOEM

#ifdef EDM_ECAT_DRIVER_IGH
    uint8_t *domain_pd_;

    Panasonic_A6B_InputDomainOffsets input_offsets_;
    Panasonic_A6B_OutputDomainOffsets output_offsets_;
#endif // EDM_ECAT_DRIVER_IGH
};

} // namespace ecat

} // namespace edm
