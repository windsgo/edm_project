#pragma once

#include "Moveruntime/Moveruntime.h"

namespace edm
{

namespace move
{

struct JumpParam {
    unit_t up_blu {0.0}; // 设定的抬刀高度 (blu)
    uint32_t dn_ms {1000}; // 设定的抬刀间隔(放电时间) (ms)
    unit_t buffer_blu {30.0}; // 抬刀缓冲距离 (blu)
    MoveRuntimePlanSpeedInput speed_param; // 抬刀速度参数
};

} // namespace move
    
} // namespace edm

