#include "CoordinateManager.h"

#include "Utils/UnitConverter/UnitConverter.h"

#include "Logger/LogMacro.h"

#include <filesystem>
#include <fstream>

#include <json.hpp> // meo json

#include <unordered_set>

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

static bool _check_json_has_int(const json::value &j,
                                const std::string &key_name,
                                bool need_positive = true) {
    auto int_find_ret = j.find(key_name);
    if (!int_find_ret) {
        return false;
    }

    if (!int_find_ret->is_number()) {
        return false;
    }

    auto int_value = int_find_ret->as_integer();
    if (need_positive && int_value < 0) {
        return false;
    }

    if ((double)int_find_ret->as_double() != int_value) {
        return false;
    }

    return true;
}

static bool _check_json_has_vector(const json::value &j,
                                   const std::string &key_name,
                                   std::size_t vec_min_size) {
    auto vec_find_ret = j.find<std::vector<double>>(key_name);
    if (!vec_find_ret) {
        return false;
    }

    if (vec_find_ret->size() < vec_min_size) {
        return false;
    }

    return true;
}

namespace json::ext {
/**
 * {
 *  "index": 54,
 *  "offset": [0, 1, 2, 3, 4, 5] //! json's offset length may be larger than the
 * size of t.offset().size()
 * }
 */
template <> class jsonization<edm::coord::Coordinate> {
public:
    json::value to_json(const edm::coord::Coordinate &t) const {
        std::vector<edm::coord::coord_offset_t::value_type> vec;
        vec.reserve(edm::coord::Coordinate::Size);
        for (std::size_t i = 0; i < edm::coord::Coordinate::Size; ++i) {
            vec.push_back(edm::util::UnitConverter::blu2mm(t.offset()[i]));
        }

        return json::object{{"index", t.index()}, {"offset", std::move(vec)}};
    }
    bool check_json(const json::value &j) const {
        if (!j.is_object()) {
            s_logger->error("Coordinate: !j.is_object()");
            return false;
        }

        if (!_check_json_has_int(j, "index", true) ||
            !_check_json_has_vector(j, "offset",
                                    edm::coord::Coordinate::Size)) {
            return false;
        }

        return true;
    }
    bool from_json(const json::value &j, edm::coord::Coordinate &out) const {
        const auto &jo = j.as_object();
        const auto &joffset_arr = jo.at("offset").as_array();
        const auto &jindex = jo.at("index");

        edm::coord::coord_offset_t offset;
        for (std::size_t i = 0; i < offset.size(); ++i) {
            offset[i] =
                edm::util::UnitConverter::mm2blu(joffset_arr[i].as_double());
        }

        out.set_index(jindex.as_unsigned());
        out.set_offset(offset);

        return true;
    }
};

template <> class jsonization<edm::coord::CoordinateManager> {
public:
    json::value to_json(const edm::coord::CoordinateManager &t) const {
        json::object jo;

        std::vector<edm::coord::coord_offset_t::value_type> global_offset_vec;
        global_offset_vec.reserve(edm::coord::Coordinate::Size);
        for (std::size_t i = 0; i < edm::coord::Coordinate::Size; ++i) {
            global_offset_vec.push_back(
                edm::util::UnitConverter::blu2mm(t.get_global_offset()[i]));
        }

        jo["global_offset"] = std::move(global_offset_vec);

        json::array c_arr{};

        for (const auto &[_, c] : t.get_coordinates_map()) {
            c_arr.push_back(c);
        }

        jo["coord_offsets"] = std::move(c_arr);

        std::vector<edm::coord::coord_offset_t::value_type> soft_limit_pos_vec;
        std::vector<edm::coord::coord_offset_t::value_type> soft_limit_neg_vec;

        soft_limit_pos_vec.reserve(edm::coord::Coordinate::Size);
        soft_limit_neg_vec.reserve(edm::coord::Coordinate::Size);
        for (std::size_t i = 0; i < edm::coord::Coordinate::Size; ++i) {
            soft_limit_pos_vec.push_back(
                edm::util::UnitConverter::blu2mm(t.get_pos_soft_limit()[i]));
            soft_limit_neg_vec.push_back(
                edm::util::UnitConverter::blu2mm(t.get_neg_soft_limit()[i]));
        }

        jo["soft_limit"] = json::object{{"pos", std::move(soft_limit_pos_vec)},
                                        {"neg", std::move(soft_limit_neg_vec)}};

        return jo;
    }

    bool check_json(const json::value &j) const {
        if (!j.is_object()) {
            return false;
        }

        if (!_check_json_has_vector(j, "global_offset",
                                    edm::coord::Coordinate::Size)) {
            return false;
        }

        auto coord_offsets_find_ret = j.find("coord_offsets");
        if (!coord_offsets_find_ret || !coord_offsets_find_ret->is_array()) {
            return false;
        }

        const auto &coord_offsets_jarr = coord_offsets_find_ret->as_array();
        if (coord_offsets_jarr.empty()) {
            return false;
        }

        // used to check if has the same index
        std::unordered_set<uint32_t> index_set;

        for (std::size_t i = 0; i < coord_offsets_jarr.size(); ++i) {
            if (!coord_offsets_jarr[i].is<edm::coord::Coordinate>()) {
                return false;
            }

            // convert the coordoffset here to fetch the index
            auto index =
                coord_offsets_jarr[i].as<edm::coord::Coordinate>().index();
            if (index_set.find(index) != index_set.end()) {
                s_logger->error("coord config has the same index: {}", index);
                return false;
            } else {
                index_set.insert(index);
            }
        }

        // check soft_limit:
        auto find_soft_limit_ret = j.find("soft_limit");
        if (!find_soft_limit_ret) {
            return false;
        }

        auto soft_limit_jv = std::move(*find_soft_limit_ret);

        if (!soft_limit_jv.is_object()) {
            return false;
        }

        if (!_check_json_has_vector(soft_limit_jv, "pos",
                                    edm::coord::Coordinate::Size)) {
            return false;
        }

        if (!_check_json_has_vector(soft_limit_jv, "neg",
                                    edm::coord::Coordinate::Size)) {
            return false;
        }

        // check limit validation (pos >= neg)
        auto pos_vec =
            soft_limit_jv.as_object()["pos"].as<std::vector<double>>();
        auto neg_vec =
            soft_limit_jv.as_object()["neg"].as<std::vector<double>>();

        for (std::size_t i = 0; i < edm::coord::Coordinate::Size; ++i) {
            if (pos_vec[i] < neg_vec[i]) {
                s_logger->error("pos_vec[{0:}] ({1:}) < neg_vec[{0:}] (2:)", i,
                                pos_vec[i], neg_vec[i]);
                return false;
            }
        }

        return true;
    }

    bool from_json(const json::value &j,
                   edm::coord::CoordinateManager &t) const {
        const auto &jo = j.as_object();
        const auto &jglobal_offset_arr = jo.at("global_offset").as_array();
        const auto &jcoord_offsets_arr = jo.at("coord_offsets").as_array();
        const auto &jsoft_limit_o = jo.at("soft_limit").as_object();
        const auto &jsoft_limit_pos_arr = jsoft_limit_o.at("pos").as_array();
        const auto &jsoft_limit_neg_arr = jsoft_limit_o.at("neg").as_array();

        edm::coord::coord_offset_t global_offset;
        for (std::size_t i = 0; i < global_offset.size(); ++i) {
            global_offset[i] = edm::util::UnitConverter::mm2blu(
                jglobal_offset_arr[i].as_double());
        }

        edm::coord::CoordinateManager::map_t coord_map;
        for (std::size_t i = 0; i < jcoord_offsets_arr.size(); ++i) {
            auto c = jcoord_offsets_arr.at(i).as<edm::coord::Coordinate>();
            if (coord_map.find(c.index()) != coord_map.end()) {
                return false;
            }

            coord_map.emplace(c.index(), c);
        }

        edm::coord::CoordSoftLimit sl;
        for (std::size_t i = 0; i < sl.pos.size(); ++i) {
            sl.pos[i] = edm::util::UnitConverter::mm2blu(
                jsoft_limit_pos_arr[i].as_double());
            sl.neg[i] = edm::util::UnitConverter::mm2blu(
                jsoft_limit_neg_arr[i].as_double());
        }

        t.clear_coordinates_map();
        t.set_global_offset(global_offset);
        t.set_coordinates_map(std::move(coord_map));

        auto set_sl_ret = t.set_soft_limits(sl);
        if (!set_sl_ret) {
            return false;
        }

        return true;
    }
};
} // namespace json::ext

namespace edm {

namespace coord {

bool CoordinateManager::save_as(const std::string &filename) const {
    return _save_to_file(filename);
}

std::optional<CoordinateManager>
CoordinateManager::LoadFromJsonFile(const std::string &filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        return std::nullopt;
    }

    auto parse_ret = json::parse(ifs, false);
    if (!parse_ret) {
        ifs.close();
        return std::nullopt;
    }

    ifs.close();

    auto root = std::move(*parse_ret);
    if (!root.is<CoordinateManager>()) {
        s_logger->error("not a CoordinateManager");
        return std::nullopt;
    }

    try {
        return root.as<CoordinateManager>();
    } catch (const std::exception &e) {
        s_logger->critical("not a CoordinateManager, ex: {}", e.what());
        return std::nullopt;
    }
}

std::vector<uint32_t> CoordinateManager::get_avaiable_coord_indexes() const {
    std::vector<uint32_t> ret;

    for (const auto &[i, _] : coordinates_map_) {
        ret.push_back(i);
    }

    return ret;
}

std::optional<coord_offset_t>
CoordinateManager::get_coord_offset(uint32_t index) const {
    auto find_ret = coordinates_map_.find(index);

    if (find_ret == coordinates_map_.end()) {
        return std::nullopt;
    }

    return find_ret->second.offset();
}

bool CoordinateManager::set_coord_offset(uint32_t coord_index,
                                         const coord_offset_t &offset) {
    auto find_coord_ret = coordinates_map_.find(coord_index);
    if (find_coord_ret == coordinates_map_.end()) {
        return false;
    }

    find_coord_ret->second.set_offset(offset);

    return true;
}

bool CoordinateManager::coord_to_machine(
    uint32_t coord_index, const move::axis_t &curr_coord_axis_value,
    move::axis_t &output) const {
    auto find_coord_ret = coordinates_map_.find(coord_index);
    if (find_coord_ret == coordinates_map_.end()) {
        return false;
    }

    find_coord_ret->second.get_machine_pos(curr_coord_axis_value, output);

    return true;
}

bool CoordinateManager::coord_to_motor(
    uint32_t coord_index, const move::axis_t &curr_coord_axis_value,
    move::axis_t &output) const {
    auto find_coord_ret = coordinates_map_.find(coord_index);
    if (find_coord_ret == coordinates_map_.end()) {
        return false;
    }

    find_coord_ret->second.get_machine_pos(curr_coord_axis_value, output);
    for (std::size_t i = 0; i < output.size(); ++i) {
        output[i] += global_offset_[i];
    }

    return true;
}

bool CoordinateManager::motor_to_machine(const move::axis_t &motor_axis,
                                         move::axis_t &output) const {
    for (std::size_t i = 0; i < output.size(); ++i) {
        output[i] = motor_axis[i] - global_offset_[i];
    }

    return true;
}

bool CoordinateManager::motor_to_coord(uint32_t coord_index,
                                       const move::axis_t &motor_axis,
                                       move::axis_t &output) const {
    auto find_coord_ret = coordinates_map_.find(coord_index);
    if (find_coord_ret == coordinates_map_.end()) {
        return false;
    }

    for (std::size_t i = 0; i < output.size(); ++i) {
        output[i] = motor_axis[i] - global_offset_[i] -
                    find_coord_ret->second.offset()[i];
    }

    return true;
}

bool CoordinateManager::machine_to_motor(const move::axis_t &machine_pos,
                                         move::axis_t &output) const {
    for (std::size_t i = 0; i < output.size(); ++i) {
        output[i] = machine_pos[i] + global_offset_[i];
    }

    return true;
}

bool CoordinateManager::set_soft_limits(const CoordSoftLimit &soft_limits) {
    // check input soft limit
    for (std::size_t i = 0; i < soft_limits.pos.size(); ++i) {
        if (soft_limits.pos[i] < soft_limits.neg[i]) {
            return false;
        }
    }

    soft_limits_ = soft_limits;
    return true;
}

bool CoordinateManager::_save_to_file(const std::string &filename) const {
    if (filename.empty()) {
        s_logger->error("{}: can not save to empty file", __PRETTY_FUNCTION__);
        return false;
    }

    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        s_logger->error("{}: can not open file: {}", __PRETTY_FUNCTION__,
                        filename);
        return false;
    }

    json::value value = *this;
    ofs << value;

    s_logger->info("coord saved to file: {}", filename);

    ofs.close();

    return true;
}

} // namespace coord

} // namespace edm
