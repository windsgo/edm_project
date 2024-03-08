#include "CoordinateManager.h"

#include "Logger/LogMacro.h"

#include <filesystem>
#include <fstream>

#include <json.hpp> // meo json

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

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
        vec.reserve(t.offset().size());
        for (std::size_t i = 0; i < t.offset().size(); ++i) {
            vec.push_back(t.offset()[i]);
        }

        return json::object{{"index", t.index()}, {"offset", std::move(vec)}};
    }
    bool check_json(const json::value &j) const {
        if (!j.is<json::object>()) {
            s_logger->error("Coordinate: !j.is<json::object>()");
            return false;
        }

        auto index_find_ret = j.find("index");
        if (!index_find_ret) {
            s_logger->error("Coordinate: index not exist");
            return false;
        }

        if (!index_find_ret->is_number()) {
            s_logger->error("Coordinate: index not number");
            return false;
        }

        if ((double)index_find_ret->as_integer() != index_find_ret->as_double()) {
            s_logger->error("Coordinate: index not integer");
            return false;
        }

        if (index_find_ret->as_integer() < 0) {
            s_logger->error("Coordinate: index not positive");
            return false;
        }

        auto offset_value_opt = j.find<std::vector<double>>("offset");
        if (!offset_value_opt) {
            s_logger->error(
                "Coordinate: offset not exist or not std::vector<double> type");
            return false;
        }
        if (offset_value_opt->size() < edm::coord::coord_offset_t{}.size()) {
            s_logger->error("Coordinate: offset size not enough: {}",
                            offset_value_opt->size());
            return false;
        }

        return true;
    }
    bool from_json(const json::value &j, edm::coord::Coordinate &out) const {
        if (!check_json(j)) {
            return false;
        }

        const auto &jo = j.as_object();
        const auto &joffset_arr = jo.at("offset").as_array();
        edm::coord::coord_offset_t offset;
        for (std::size_t i = 0; i < offset.size(); ++i) {
            offset[i] = joffset_arr[i].as_double();
        }
        out = std::move(
            edm::coord::Coordinate{jo.at("index").as_unsigned(), offset});

        return true;
    }
};
} // namespace json::ext

static std::optional<std::string> _create_backup_file_for(const std::string ori_file) {
    auto bak_file = ori_file + ".bak";
    if (std::filesystem::exists(bak_file)) {
        int i = 1;
        while (true || i > 20) {
            auto bak_i_file = bak_file + "." + std::to_string(i);
            if (!std::filesystem::exists(bak_i_file)) {
                bak_file = std::move(bak_i_file);
                break;
            }
            ++i;
        }
    }

    auto ret = std::filesystem::copy_file(ori_file, bak_file);
    if (ret) {
        s_logger->info("successfully create coord backup file: {}", bak_file);
        return std::move(bak_file);
    } else {
        s_logger->error("create coord backup file failed for: {}", ori_file);
        return std::nullopt;
    }
}

namespace edm {

namespace coord {

void CoordinateManager::save_as(const std::string & filename) const {
    _save_to_file(filename);
}

void CoordinateManager::set_filename(const std::string &new_filename) {
    filename_ = new_filename;

    _save_to_file(filename_);
}

CoordinateManager::CoordinateManager(const std::string &filename) : filename_(filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        s_logger->warn("CoordinateManager init file open failed: {}", filename);
        _reset_and_generate_default_coord();
        return;
    }

    auto parse_ret = json::parse(ifs, false);
    if (!parse_ret) {
        s_logger->warn(
            "CoordinateManager init file parse failed, json syntax error: {}",
            filename);
        _reset_and_generate_default_coord();
        ifs.close();
        _create_backup_file_for(filename);
        return;
    }

    auto root = std::move(*parse_ret);
    if (!_parse_json_to_map(root)) {
        s_logger->warn(
            "CoordinateManager init file parse failed, coord error: {}",
            filename);
        _reset_and_generate_default_coord();
        ifs.close();
        _create_backup_file_for(filename);
        return;
    }

    s_logger->debug("CoordinateManager init success, size: {}",
                    coordinates_map_.size());

    ifs.close();
}

bool CoordinateManager::_parse_json_to_map(const json::value &value) {
    if (!value.is_array()) {
        s_logger->error("{}, not array", __PRETTY_FUNCTION__);
        return false;
    }

    for (const auto &i : value.as_array()) {
        try {
            Coordinate c = (Coordinate)i;
            if (coordinates_map_.find(c.index()) != coordinates_map_.end()) {
                s_logger->error("{}, repeat index: {}", __PRETTY_FUNCTION__, c.index());
                coordinates_map_.clear();
                return false;
            }
            coordinates_map_.emplace(c.index(), c);
        } catch (const std::exception &e) {
            s_logger->error("{}, exception : {}", __PRETTY_FUNCTION__,
                            e.what());
            coordinates_map_.clear();
            return false;
        }
    }

    return true;
}

void CoordinateManager::_reset_and_generate_default_coord() {
    // reset
    coordinates_map_.clear();

    // generate default coordiates: g54 ~ g59 by default
    coordinates_map_.reserve(59 - 54 + 1);
    for (uint32_t i = 54; i <= 59; ++i) {
        coordinates_map_.emplace(i, Coordinate{i, coord_offset_t{0.0}});
    }
}

void CoordinateManager::_save_to_file(const std::string &filename) const {
    if (filename.empty()) {
        s_logger->error("{}: can not save to empty file", __PRETTY_FUNCTION__);
        return;
    }

    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        s_logger->error("{}: can not open file: {}", __PRETTY_FUNCTION__,
                        filename);
        return;
    }
    
    auto value = _to_json();
    ofs << value;

    s_logger->info("coord saved to file: {}", filename);

    ofs.close();
}

json::value CoordinateManager::_to_json() const {
    json::array jo;
    for (const auto& [_, c] : coordinates_map_) {
        json::value v {c};
        jo.push_back(std::move(v));
    }

    return std::move(jo);
}

} // namespace coord

} // namespace edm
