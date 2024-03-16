#include "GCodeTaskConverter.h"

#include <unordered_map>

#include "config.h"

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER_NAME(s_logger, "interp");

namespace edm {

namespace task {

static const std::unordered_map<std::string, GCodeTaskType> s_type_map = {

#define XX_(type__) \
    { #type__, GCodeTaskType::type__ }

    XX_(Undefined),
    XX_(G00MotionCommand),
    XX_(G01MotionCommand),
    XX_(CoordinateModeCommand),
    XX_(CoordinateIndexCommand),
    XX_(G00MotionCommand),
    XX_(EleparamSetCommand),
    XX_(FeedSpeedSetCommand),
    XX_(DelayCommand),
    XX_(PauseCommand),
    XX_(ProgramEndCommand)

#undef XX_

};

static const std::unordered_map<std::string, GCodeCoordinateMode>
    s_coord_mode_map = {

#define XX_(type__) \
    { #type__, GCodeCoordinateMode::type__ }

        XX_(Undefined), XX_(AbsoluteMode), XX_(IncrementMode)

#undef XX_

};

static std::optional<GCodeTaskBase::ptr> _make_g00(const json::object &jo) {
    // may throw exception
    auto coord_mode_str = jo.at("CoordinateMode").as_string();

    auto coord_mode_enum_find_ret = s_coord_mode_map.find(coord_mode_str);
    if (coord_mode_enum_find_ret == s_coord_mode_map.end()) {
        s_logger->error("GCodeTaskConverter _make_g00: CoordinateMode err: {}",
                        coord_mode_str);
        return std::nullopt;
    }

    auto coord_mode = coord_mode_enum_find_ret->second;

    auto coord_index = jo.at("CoordinateIndex").as_integer();

    auto line_number = jo.at("LineNumber").as_integer();

    auto m05 = jo.at("M05IgnoreTouchDetect").as_boolean();

    auto feed_speed = jo.at("FeedSpeed").as_integer();

    const auto &coords = jo.at("Coordinates").as_array();
    if (coords.size() < 6) {
        s_logger->error("GCodeTaskConverter _make_g00: coords less than 6, {}",
                        coords.size());
        return std::nullopt;
    }

    std::vector<std::optional<double>> values;
    values.resize(6);
    for (std::size_t i = 0; i < 6; ++i) {
        if (!coords[i].is_null()) {
            values[i] = coords[i].as_double();
        }
    }

    auto g00 = std::make_shared<GCodeTaskG00Motion>(
        !m05, feed_speed, coord_index, coord_mode, values, line_number, -1);

    return g00;
}

static std::optional<GCodeTaskBase::ptr> _make_g01(const json::object &jo) {
    // may throw exception
    auto coord_mode_str = jo.at("CoordinateMode").as_string();

    auto coord_mode_enum_find_ret = s_coord_mode_map.find(coord_mode_str);
    if (coord_mode_enum_find_ret == s_coord_mode_map.end()) {
        s_logger->error("GCodeTaskConverter _make_g01: CoordinateMode err: {}",
                        coord_mode_str);
        return std::nullopt;
    }

    auto coord_mode = coord_mode_enum_find_ret->second;

    auto coord_index = jo.at("CoordinateIndex").as_integer();

    auto line_number = jo.at("LineNumber").as_integer();

    const auto &coords = jo.at("Coordinates").as_array();
    if (coords.size() < 6) {
        s_logger->error("GCodeTaskConverter _make_g01: coords less than 6, {}",
                        coords.size());
        return std::nullopt;
    }

    std::vector<std::optional<double>> values;
    for (std::size_t i = 0; i < 6; ++i) {
        if (!coords[i].is_null()) {
            values[i] = coords[i].as_double();
        }
    }

    auto g01 = std::make_shared<GCodeTaskG01Motion>(coord_index, coord_mode,
                                                    values, line_number, -1);

    return g01;
}

static std::optional<GCodeTaskBase::ptr> _make_coord_index(const json::object &jo) {
    auto coord_index = jo.at("CoordinateIndex").as_integer();

    auto line_number = jo.at("LineNumber").as_integer();

    auto coord_index_task = std::make_shared<GCodeTaskCoordinateIndex>(coord_index, line_number, -1);

    return coord_index_task;
}

static std::optional<GCodeTaskBase::ptr> _make_ele_set(const json::object &jo) {
    auto ele_index = jo.at("EleparamIndex").as_integer();

    auto line_number = jo.at("LineNumber").as_integer();

    auto ele_set = std::make_shared<GCodeTaskEleparamSet>(ele_index, line_number, -1);

    return ele_set;
}

static std::optional<GCodeTaskBase::ptr> _make_delay(const json::object &jo) {
    auto delay_time = jo.at("DelayTime").as_double();

    auto line_number = jo.at("LineNumber").as_integer();

    auto delay = std::make_shared<GCodeTaskDeley>(delay_time, line_number, -1);

    return delay;
}

static std::optional<GCodeTaskBase::ptr> _make_m02(const json::object &jo) {
    auto line_number = jo.at("LineNumber").as_integer();

    auto m02 = std::make_shared<GCodeTaskProgramEnd>(line_number, -1);

    return m02;
}

static std::optional<GCodeTaskBase::ptr> _make_m00(const json::object &jo) {
    auto line_number = jo.at("LineNumber").as_integer();

    auto m00 = std::make_shared<GCodeTaskPauseCommand>(line_number, -1);

    return m00;
}

std::optional<GCodeTaskBase::ptr>
GCodeTaskConverter::_MakeGCodeTaskFromJsonObject(const json::object &jo) {

    // 不作异常处理, 外层做
    auto type_str = jo.at("CommandType").as_string();

    auto type_enum_find_ret = s_type_map.find(type_str);
    if (type_enum_find_ret == s_type_map.end()) {
        s_logger->error("GCodeTaskConverter: type err: {}", type_str);
        return std::nullopt; // failed
    }

    auto type = static_cast<GCodeTaskType>(type_enum_find_ret->second);
    s_logger->debug("GCodeTaskConverter: type {},{}", type_str, (int)type);

    switch (type) {
    case GCodeTaskType::Undefined:
        s_logger->error("GCodeTaskConverter: type is undefined.");
        return std::nullopt; // failed

    case GCodeTaskType::G00MotionCommand: {
        return _make_g00(jo);
    }

    case GCodeTaskType::G01MotionCommand: {
        return _make_g01(jo);
    }
    
    case GCodeTaskType::CoordinateIndexCommand: {
        return _make_coord_index(jo);
    }

    case GCodeTaskType::EleparamSetCommand: {
        return _make_ele_set(jo);
    }

    case GCodeTaskType::DelayCommand: {
        return _make_delay(jo);
    }

    case GCodeTaskType::ProgramEndCommand: {
        return _make_m02(jo);
    }

    case GCodeTaskType::PauseCommand: {
        return _make_m00(jo);
    }

    default:
        s_logger->info("Ignore task: {}", type_str);
        return nullptr; // ignored
    }
}

std::optional<std::vector<GCodeTaskBase::ptr>>
GCodeTaskConverter::MakeGCodeTaskListFromJson(const json::value &j) {

    std::vector<GCodeTaskBase::ptr> gcode_task_list;
    
    if (!j.is_array()) {
        s_logger->error("MakeGCodeTaskListFromJson: j is not an array, type: {}", (int)j.type());
        return std::nullopt;
    }

    const auto& jarr = j.as_array();
    if (jarr.empty()) {
        s_logger->error("MakeGCodeTaskListFromJson: jarr is empty");
        return std::nullopt;
    }

    gcode_task_list.reserve(jarr.size()); // 预留空间, 会多出来, 有些node会被忽略

    for (std::size_t i = 0; i < jarr.size(); ++i) {
        if (!jarr[i].is_object()) {
            s_logger->error("MakeGCodeTaskListFromJson: jarr[{}] is not empty", i);
            return std::nullopt;
        }

        try {
            auto ret = _MakeGCodeTaskFromJsonObject(jarr[i].as_object());
            if (!ret) {
                // failed
                s_logger->error("MakeGCodeTaskListFromJson: process jarr[{}] failed", i);
                return std::nullopt;
            }

            if ((*ret)) {
                // not ignored (not nullptr)
                gcode_task_list.push_back(*ret);
            }

        } catch (const std::exception& e) {
            s_logger->error("MakeGCodeTaskListFromJson exception: what(): {}", e.what());
            return std::nullopt;
        }
    }

    return gcode_task_list;
}

} // namespace task

} // namespace edm
