#include "Coordinate/CoordinateManager.h"

#include <iostream>
#include <optional>
#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

#include "config.h"

using namespace edm::coord;
using namespace edm::move;

int main () {

    std::string file = EDM_CONFIG_DIR "coord.json";

    auto cm_opt = CoordinateManager::LoadFromJsonFile(file);

    if (!cm_opt) {
        s_logger->error("create failed");

        auto cm = CoordinateManager{};

        s_logger->info("size: {}, save_ret: {}", cm.size(), cm.save_as(EDM_CONFIG_DIR "hi.json"));

        return 1;
    }

    auto cm = std::move(*cm_opt);

    s_logger->info("size: {}, save_ret: {}", cm.size(), cm.save_as(EDM_CONFIG_DIR "hi.json"));

    axis_t input {10, 10, 10};
    axis_t motor_pos;
    axis_t output;

    // 获取真实坐标(绝对坐标, 电机坐标)
    cm.coord_to_motor(54, input, motor_pos);
    s_logger->info("motor_pos: ({}, {}, {})", motor_pos[0], motor_pos[1], motor_pos[2]);

    // 获取机床坐标
    cm.coord_to_machine(54, input, output);
    s_logger->info("machine1: ({}, {}, {})", output[0], output[1], output[2]);

    cm.set_coord_offset(54, output);
    cm.motor_to_coord(54, motor_pos, output);
    s_logger->info("g54 after set offset: ({}, {}, {})", output[0], output[1], output[2]);

#if 0 
    // 第二种方式(从电机坐标获取)
    cm.motor_to_machine(motor_pos, output);
    s_logger->info("machine2: ({}, {}, {})", output[0], output[1], output[2]);

    // 将当前点设置为机床坐标系原点 (即将当前电机坐标值设置为全局偏置)
    cm.set_global_offset(motor_pos);

    // 获取原先点的机床坐标值 应当为0,0,0
    cm.motor_to_machine(motor_pos, output);
    s_logger->info("machine after set global offset: ({}, {}, {})", output[0], output[1], output[2]);
#endif 


    // const auto& d = cm.data();

    // s_logger->debug("filename: {}", cm.get_filename());
    // for (const auto& [i, c] : d) {
    //     s_logger->debug("{}: {}, {}", i, c.index(), c.offset()[0]);
    // } 

    // cm.save_as(EDM_CONFIG_DIR  "coord6.json");

    // auto ccc = std::make_shared<CoordinateManager>(EDM_CONFIG_DIR "coord6.json");

    return 0;
}