#pragma once

#include <memory>
#include <string>

// json serialize
#include <json.hpp>

namespace edm
{

namespace ecat
{    

// TODOs 

struct SettingsDefine {
    std::string netif_name;
    std::size_t iomap_size {4096};
    std::size_t servo_num {0};
    std::size_t io_num {0};

    MEO_JSONIZATION(netif_name, iomap_size, servo_num, MEO_OPT io_num);
};

struct ServoDeviceDefine {
    std::size_t index; // 0, 1, 2, ...
    std::string name; // "x", "y", ...
    std::string type; // "panasonic", ...
    uint32_t max_acc {500000}; // um/s^2

    MEO_JSONIZATION(index, name, type, max_acc);
};

struct IODeviceDefine {
    uint32_t dummy;
    MEO_JSONIZATION(MEO_OPT dummy);
};

struct EcatDeviceDefine {
    std::string type {"servo"}; // "servo" or "io"
    ServoDeviceDefine servo_define;
    IODeviceDefine io_define;

    MEO_JSONIZATION(type, MEO_OPT servo_define, MEO_OPT io_define);
};

struct TotalEcatDefine {
    SettingsDefine settings;
    std::vector<EcatDeviceDefine> ecats;

    MEO_JSONIZATION(settings, MEO_OPT ecats);
};

} // namespace ecat

} // namespace edm
