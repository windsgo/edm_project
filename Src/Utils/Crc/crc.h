#pragma once

#include <cstdint>

namespace edm {

namespace util {

// Modbus Use
uint16_t modbus_crc_table_calc(const uint8_t *ptr, int len);

// Tcp Use
uint16_t tcp_crc_table_calc(const uint8_t *ptr, int len);

}

}