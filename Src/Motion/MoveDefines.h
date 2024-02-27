#pragma once

#include <cstdint>
#include <array>
#include <vector>

#include "config.h"

namespace edm
{

namespace move
{

using unit_t = double;

using axis_t = std::array<unit_t, EDM_AXIS_NUM>;

} // namespace move

} // namespace edm
