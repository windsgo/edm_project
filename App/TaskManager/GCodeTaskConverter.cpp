#include "GCodeTaskConverter.h"

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "Motion/MoveDefines.h"
#include "TaskManager/GCodeTask.h"
#include "TaskManager/GCodeTaskBase.h"
#include "Utils/UnitConverter/UnitConverter.h"
#include "common/utils.hpp"
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
    XX_(EleparamSetCommand),
    XX_(FeedSpeedSetCommand),
    XX_(DelayCommand),
    XX_(PauseCommand),
    XX_(ProgramEndCommand),
    XX_(CoordSetZeroCommand),
    XX_(DrillMotionCommand),
    XX_(G01GroupMotionCommand)

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

    auto touch = jo.at("G00Touch").as_boolean();

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

    auto g00 = std::make_shared<GCodeTaskG00Motion>(!m05, touch, feed_speed,
                                                    coord_index, coord_mode,
                                                    values, line_number, -1);
    
    // if (jo.contains("CommandStr")) {
    //     g00->set_gcode_str(jo.at("CommandStr").as_string());
    // }

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
    values.resize(6);
    for (std::size_t i = 0; i < 6; ++i) {
        if (!coords[i].is_null()) {
            values[i] = coords[i].as_double();
        }
    }

    auto g01 = std::make_shared<GCodeTaskG01Motion>(coord_index, coord_mode,
                                                    values, line_number, -1);
    
    // if (jo.contains("CommandStr")) {
    //     g01->set_gcode_str(jo.at("CommandStr").as_string());
    // }

    return g01;
}

static std::optional<GCodeTaskBase::ptr>
_make_coord_index(const json::object &jo) {
    auto coord_index = jo.at("CoordinateIndex").as_integer();

    auto line_number = jo.at("LineNumber").as_integer();

    auto coord_index_task = std::make_shared<GCodeTaskCoordinateIndex>(
        coord_index, line_number, -1);
    
    // if (jo.contains("CommandStr")) {
    //     coord_index_task->set_gcode_str(jo.at("CommandStr").as_string());
    // }

    return coord_index_task;
}

static std::optional<GCodeTaskBase::ptr> _make_ele_set(const json::object &jo) {
    auto ele_index = jo.at("EleparamIndex").as_integer();

    auto line_number = jo.at("LineNumber").as_integer();

    auto ele_set =
        std::make_shared<GCodeTaskEleparamSet>(ele_index, line_number, -1);

    // if (jo.contains("CommandStr")) {
    //     ele_set->set_gcode_str(jo.at("CommandStr").as_string());
    // }

    return ele_set;
}

static std::optional<GCodeTaskBase::ptr> _make_delay(const json::object &jo) {
    auto delay_time = jo.at("DelayTime").as_double();

    auto line_number = jo.at("LineNumber").as_integer();

    auto delay = std::make_shared<GCodeTaskDeley>(delay_time, line_number, -1);

    // if (jo.contains("CommandStr")) {
    //     delay->set_gcode_str(jo.at("CommandStr").as_string());
    // }

    return delay;
}

static std::optional<GCodeTaskBase::ptr> _make_m02(const json::object &jo) {
    auto line_number = jo.at("LineNumber").as_integer();

    auto m02 = std::make_shared<GCodeTaskProgramEnd>(line_number, -1);

    // if (jo.contains("CommandStr")) {
    //     m02->set_gcode_str(jo.at("CommandStr").as_string());
    // }

    return m02;
}

static std::optional<GCodeTaskBase::ptr> _make_m00(const json::object &jo) {
    auto line_number = jo.at("LineNumber").as_integer();

    auto m00 = std::make_shared<GCodeTaskPauseCommand>(line_number, -1);

    // if (jo.contains("CommandStr")) {
    //     m00->set_gcode_str(jo.at("CommandStr").as_string());
    // }

    return m00;
}

static std::optional<GCodeTaskBase::ptr>
_make_coord_mode(const json::object &jo) {
    auto line_number = jo.at("LineNumber").as_integer();

    auto coord_mode =
        std::make_shared<GCodeTaskCoordinateMode>(line_number, -1);

    // if (jo.contains("CommandStr")) {
    //     coord_mode->set_gcode_str(jo.at("CommandStr").as_string());
    // }

    return coord_mode;
}

static std::optional<GCodeTaskBase::ptr>
_make_feed_set(const json::object &jo) {
    auto line_number = jo.at("LineNumber").as_integer();

    auto feed_set = std::make_shared<GCodeTaskFeedSpeedSet>(line_number, -1);

    // if (jo.contains("CommandStr")) {
    //     feed_set->set_gcode_str(jo.at("CommandStr").as_string());
    // }

    return feed_set;
}

static std::optional<GCodeTaskBase::ptr>
_make_coord_set_zero(const json::object &jo) {
    auto line_number = jo.at("LineNumber").as_integer();

    const auto &j_set_zero_axis_list = jo.at("SetZeroAxisList").as_array();
    if (j_set_zero_axis_list.size() < 6) {
        s_logger->error(
            "GCodeTaskConverter _make_coord_set_zero: less than 6, {}",
            j_set_zero_axis_list.size());
        return std::nullopt;
    }

    std::vector<bool> set_zero_axis_list;
    set_zero_axis_list.resize(6);
    for (std::size_t i = 0; i < 6; ++i) {
        if (j_set_zero_axis_list[i].is_boolean()) {
            set_zero_axis_list[i] = j_set_zero_axis_list[i].as_boolean();
        } else {
            s_logger->error(
                "GCodeTaskConverter _make_coord_set_zero: axis {} not boolean",
                i);
            return std::nullopt;
        }
    }

    auto coord_set_zero = std::make_shared<GCodeTaskCoordSetZeroCommand>(
        set_zero_axis_list, line_number, -1);

    // if (jo.contains("CommandStr")) {
    //     coord_set_zero->set_gcode_str(jo.at("CommandStr").as_string());
    // }

    return coord_set_zero;
}

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
static std::optional<GCodeTaskBase::ptr> _make_drill(const json::object &jo) {
    move::DrillStartParams start_params;

    auto line_number = jo.at("LineNumber").as_integer();

    start_params.depth_um =
        util::UnitConverter::mm2um(jo.at("DrillDepth").as_double());
    start_params.holdtime_ms = jo.at("DrillHoldTime").as_integer();
    start_params.touch = jo.at("DrillTouch").as_boolean();
    start_params.breakout = jo.at("DrillBreakout").as_boolean();
    start_params.back = jo.at("DrillBack").as_boolean();

    const auto &jv_spindle_speed = jo.at("DrillSpindleSpeed");
    if (jv_spindle_speed.is_null()) {
        start_params.spindle_speed_blu_ms_opt = std::nullopt;
    } else {
        start_params.spindle_speed_blu_ms_opt = jv_spindle_speed.as_double();
    }

    auto drill =
        std::make_shared<GCodeTaskDrillMotion>(start_params, line_number, -1);

    // if (jo.contains("CommandStr")) {
    //     drill->set_gcode_str(jo.at("CommandStr").as_string());
    // }

    return drill;
}
#endif

static std::optional<GCodeTaskBase::ptr>
_make_g01_group(const json::object &jo) {
    std::vector<GCodeTaskG01GroupMotion::G01GroupPoint> point_vec;

    auto coord_index = jo.at("CoordinateIndex").as_integer();

    auto line_number = jo.at("LineNumber").as_integer();

    auto points_ja = jo.at("G01GroupPoints").as_array();
    // s_logger->debug("G01GroupPoints size: {}", points_ja.size());

    for (const auto &item_jv : points_ja) {
        const auto& item_jo = item_jv.as_object();
        // s_logger->debug("G01GroupPoint: {}", item_jo.dumps());

        auto coord_mode_str = item_jo.at("CoordinateMode").as_string();

        auto coord_mode_enum_find_ret = s_coord_mode_map.find(coord_mode_str);
        if (coord_mode_enum_find_ret == s_coord_mode_map.end()) {
            s_logger->error(
                "GCodeTaskConverter _make_g01_group: CoordinateMode err: {}",
                coord_mode_str);
            return std::nullopt;
        }

        auto coord_mode = coord_mode_enum_find_ret->second;

        GCodeTaskG01GroupMotion::G01GroupPoint point;
        point.coord_mode = coord_mode;

        point.line_number = item_jo.at("LineNumber").as_integer();

        const auto &coords = item_jo.at("Coordinates").as_array();
        if (coords.size() < 6) {
            s_logger->error(
                "GCodeTaskConverter _make_g01_group: coords less than 6, {}",
                coords.size());
            return std::nullopt;
        }

        point.cmd_values.resize(6);
        for (std::size_t i = 0; i < 6; ++i) {
            if (!coords[i].is_null()) {
                point.cmd_values[i] = coords[i].as_double();
            }
        }
        point_vec.push_back(point);
    }

    // test print
    // for (int i = 0; i < point_vec.size(); ++i) {
    //     s_logger->debug("G01GroupPoint[{}]:", i);
    //     s_logger->debug("  LineNumber: {}", point_vec[i].line_number);
    //     s_logger->debug("  CoordinateMode: {}", (int)point_vec[i].coord_mode);
    //     for (int j = 0; j < 6; ++j) {
    //         if (point_vec[i].cmd_values[j]) {
    //             s_logger->debug("  {}: {}", j, *(point_vec[i].cmd_values[j]));
    //         } else {
    //             s_logger->debug("  {}: null", j);
    //         }
    //     }
    // }

    auto g01_group = std::make_shared<GCodeTaskG01GroupMotion>(
        coord_index, point_vec, line_number, -1);

    // if (jo.contains("CommandStr")) {
    //     g01_group->set_gcode_str(jo.at("CommandStr").as_string());
    // }

    return g01_group;
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

    std::optional<GCodeTaskBase::ptr> make_ret { std::nullopt };

    switch (type) {
    case GCodeTaskType::Undefined:
        s_logger->error("GCodeTaskConverter: type is undefined.");
        make_ret = std::nullopt; // failed
        break;

    case GCodeTaskType::G00MotionCommand: {
        make_ret = _make_g00(jo);
        break;
    }

    case GCodeTaskType::G01MotionCommand: {
        make_ret = _make_g01(jo);
        break;
    }

    case GCodeTaskType::CoordinateIndexCommand: {
        make_ret = _make_coord_index(jo);
        break;
    }

    case GCodeTaskType::EleparamSetCommand: {
        make_ret = _make_ele_set(jo);
        break;
    }

    case GCodeTaskType::DelayCommand: {
        make_ret = _make_delay(jo);
        break;
    }

    case GCodeTaskType::ProgramEndCommand: {
        make_ret = _make_m02(jo);
        break;
    }

    case GCodeTaskType::PauseCommand: {
        make_ret = _make_m00(jo);
        break;
    }
    case GCodeTaskType::CoordSetZeroCommand: {
        make_ret = _make_coord_set_zero(jo);
        break;
    }

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    // 不是小孔时, 忽略此条
    case GCodeTaskType::DrillMotionCommand: {
        make_ret = _make_drill(jo);
        break;
    }
#endif

    case GCodeTaskType::G01GroupMotionCommand: {
        make_ret = _make_g01_group(jo);
        break;
    }

    //! 以下命令虽然构造, 但是实际无操作
    // G90G91的设置记录在G00node中
    // F值的设置也记录在G00node中
    case GCodeTaskType::CoordinateModeCommand: {
        make_ret = _make_coord_mode(jo);
        break;
    }
    case GCodeTaskType::FeedSpeedSetCommand: {
        make_ret = _make_feed_set(jo);
        break;
    }

    default:
        s_logger->warn("Ignore task: {}", type_str);
        make_ret = nullptr; // ignored
        break;
    }

    if (!make_ret) {
        // failed (std::nullopt)
        return make_ret;
    }

    if (!make_ret.value()) {
        // ignored (nullptr)
        return make_ret;
    }

    // not failed and not ignored

    if (jo.contains("CommandStr")) {
        if (!jo.at("CommandStr").is_string()) {
            s_logger->error(
                "MakeGCodeTaskListFromJson: CommandStr is not a string");
            return std::nullopt; // failed
        }

        (*make_ret)->set_gcode_str(jo.at("CommandStr").as_string());
    }

    if (jo.contains("Options")) {
        auto&& jv_options = jo.at("Options");
        
        if (!jv_options.is_array()) {
            s_logger->error(
                "MakeGCodeTaskListFromJson: Options is not an array");
            return std::nullopt; // failed
        }

        auto&& ja_options = jv_options.as_array();
        std::vector<std::string> options_vec;
        for (const auto &jv_option : ja_options) {
            if (!jv_option.is_string()) {
                s_logger->error(
                    "MakeGCodeTaskListFromJson: Options[{}] is not a string");
                return std::nullopt; // failed
            }

            options_vec.push_back(jv_option.as_string());
            // s_logger->debug("option: {}", jv_option.as_string());
        }
        (*make_ret)->set_options(options_vec);
    }

    return make_ret;
}

std::optional<std::vector<GCodeTaskBase::ptr>>
GCodeTaskConverter::MakeGCodeTaskListFromJson(const json::value &j) {

    std::vector<GCodeTaskBase::ptr> gcode_task_list;

    if (!j.is_array()) {
        s_logger->error(
            "MakeGCodeTaskListFromJson: j is not an array, type: {}",
            (int)j.type());
        return std::nullopt;
    }

    const auto &jarr = j.as_array();
    if (jarr.empty()) {
        s_logger->error("MakeGCodeTaskListFromJson: jarr is empty");
        return std::nullopt;
    }

    gcode_task_list.reserve(
        jarr.size()); // 预留空间, 会多出来, 有些node会被忽略

    for (std::size_t i = 0; i < jarr.size(); ++i) {
        if (!jarr[i].is_object()) {
            s_logger->error("MakeGCodeTaskListFromJson: jarr[{}] is not empty",
                            i);
            return std::nullopt;
        }

        try {
            auto ret = _MakeGCodeTaskFromJsonObject(jarr[i].as_object());
            if (!ret) {
                // failed
                s_logger->error(
                    "MakeGCodeTaskListFromJson: process jarr[{}] failed", i);
                return std::nullopt;
            }

            if ((*ret)) {
                // not ignored (not nullptr)
                gcode_task_list.push_back(*ret);
            }

        } catch (const std::exception &e) {
            s_logger->error("MakeGCodeTaskListFromJson exception: what(): {}",
                            e.what());
            return std::nullopt;
        }
    }

    return gcode_task_list;
}

} // namespace task

} // namespace edm
