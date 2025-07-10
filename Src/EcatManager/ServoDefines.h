#pragma once

#include <cstdint>

#if defined(__GNUC__)
#ifndef PACKED
#define PACKED_BEGIN
#define PACKED __attribute__((__packed__))
#define PACKED_ALL
#define PACKED_END
#endif // PACKED
#elif defined(__CC_ARM)
#ifndef PACKED
#define PACKED_BEGIN __packed
#define PACKED
#define PACKED_ALL __packed
#define PACKED_END
#endif // PACKED
#else  // not defined (__GNUC__) or defined(__ARM_CC)
#error "error: no packed defined"
#ifndef PACKED
#define PACKED_BEGIN
#define PACKED
#define PACKED_ALL
#define PACKED_END
#endif // PACKED
#endif // defined (__GNUC__) or defined(__ARM_CC)

namespace edm {

namespace ecat {

enum class ServoType {
    UnknownType = 0,
    Panasonic_A5B = 1,
    Panasonic_A5B_WithVOffset = 2 // 带速度偏执
};

// IGH
#define PanaSonic_A6B_VendorID    0x0000066f
#define PanaSonic_A6B_ProductCode 0x60380006

struct Panasonic_A6B_OutputDomainOffsets {
    uint32_t off_control_word;
    uint32_t off_modes_of_operation;
    uint32_t off_target_position;
    // uint32_t off_touch_probe_function;
    uint32_t off_v_offset;
};

struct Panasonic_A6B_InputDomainOffsets {
    // uint32_t off_error_code;
    uint32_t off_status_word;
    // uint32_t off_modes_of_operation_display;
    uint32_t off_position_actual_value;
    // uint32_t off_touch_probe_status;
    // uint32_t off_touch_probe_pos1_pos_value;
    uint32_t off_following_error_actual_value;
    // uint32_t off_digital_inputs;
};

// SOEM

PACKED_BEGIN
struct PACKED Panasonic_A5B_Ctrl {
    uint16_t control_word;
    uint8_t modes_of_operation;
    int32_t target_position;
    uint16_t touch_probe_function;
    // int32_t v_offset;
};
PACKED_END

PACKED_BEGIN
struct PACKED Panasonic_A5B_Stat {
    uint16_t error_code;
    uint16_t status_word;
    uint8_t modes_of_operation_display;
    int32_t position_actual_value;
    uint16_t touch_probe_status;
    int32_t touch_probe_pos1_pos_value;
    int32_t following_error_actual_value;
    uint32_t digital_inputs;
};
PACKED_END

} // namespace ecat

} // namespace edm
