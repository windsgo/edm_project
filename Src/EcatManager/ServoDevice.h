#pragma once

#include <cstdint>
#include <memory>

#include "ServoDefines.h"
#include "Panasonic.h"

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

    virtual uint16_t get_status_word() = 0;
    virtual int32_t get_actual_position() = 0;

    virtual bool sw_fault() const = 0;
    virtual bool sw_switch_on_disabled() const = 0;
    virtual bool sw_ready_to_switch_on() const = 0;
    virtual bool sw_switched_on() const = 0;
    virtual bool sw_operational_enabled() const = 0;

    virtual void cw_fault_reset() = 0;
    virtual void cw_shut_down() = 0;
    virtual void cw_switch_on() = 0;
    virtual void cw_enable_operation() = 0;

    virtual void sync_actual_position_to_target_position() = 0;
};

class PanasonicServoDevice final : public ServoDevice {
public:
    PanasonicServoDevice(Panasonic_A5B_Ctrl *ctrl,
                         Panasonic_A5B_Stat *stat) noexcept;
    virtual ~PanasonicServoDevice() noexcept = default;

    PanasonicServoDevice(const PanasonicServoDevice&) = delete;
    PanasonicServoDevice& operator=(const PanasonicServoDevice&) = delete;
    PanasonicServoDevice(PanasonicServoDevice&&) = delete;
    PanasonicServoDevice& operator=(PanasonicServoDevice&&) = delete;

    constexpr ServoType type() const override {
        return ServoType::Panasonic_A5B;
    }

    void set_control_word(uint16_t control_word) override;
    void set_target_position(int32_t target_posistion) override;
    void set_operation_mode(uint8_t operation_mode) override;

    uint16_t get_status_word() override;
    int32_t get_actual_position() override;

    bool sw_fault() const override;
    bool sw_switch_on_disabled() const override;
    bool sw_ready_to_switch_on() const override;
    bool sw_switched_on() const override;
    bool sw_operational_enabled() const override;

    void cw_fault_reset() override;
    void cw_shut_down() override;
    void cw_switch_on() override;
    void cw_enable_operation() override;

    void sync_actual_position_to_target_position() override;

private:
    volatile Panasonic_A5B_Ctrl *ctrl_;
    volatile Panasonic_A5B_Stat *stat_;
};

} // namespace ecat

} // namespace edm
