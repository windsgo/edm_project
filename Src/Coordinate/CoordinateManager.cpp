#include "CoordinateManager.h"

#include "Logger/LogMacro.h"

#include <filesystem>
#include <fstream>

#include <json.hpp> // meo json

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
        vec.reserve(t.offset().size());
        for (std::size_t i = 0; i < t.offset().size(); ++i) {
            vec.push_back(t.offset()[i]);
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
            offset[i] = joffset_arr[i].as_double();
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
        global_offset_vec.reserve(t.get_global_offset().size());
        for (std::size_t i = 0; i < t.get_global_offset().size(); ++i) {
            global_offset_vec.push_back(t.get_global_offset()[i]);
        }

        jo["global_offset"] = json::array{std::move(global_offset_vec)};

        json::array c_arr{};

        for (const auto &[_, c] : t.get_coordinates_map()) {
            c_arr.push_back(c);
        }

        jo["coord_offsets"] = std::move(c_arr);

        return jo;
    }

    bool check_json(const json::value &j) const {
        if (!j.is_object()) {
            return false;
        }

        if (!_check_json_has_vector(j, "global_offset", edm::coord::Coordinate::Size)) {
            return false;
        }

        auto coord_offsets_find_ret = j.find("coord_offsets");
        if (!coord_offsets_find_ret || !coord_offsets_find_ret->is_array()) {
            return false;
        }

        const auto& coord_offsets_jarr = coord_offsets_find_ret->as_array();
        for (std::size_t i = 0; i < coord_offsets_jarr.size(); ++i) {
            if (!coord_offsets_jarr[i].is<edm::coord::Coordinate>()) {
                return false;
            }
        } 

        return true;
    };

    bool from_json(const json::value& j, edm::coord::CoordinateManager& t) const {
        const auto& jo = j.as_object();
        const auto& jglobal_offset_arr = jo.at("global_offset").as_array();
        const auto& jcoord_offsets_arr = jo.at("coord_offsets").as_array();

        edm::coord::coord_offset_t global_offset;
        for (std::size_t i = 0; i < global_offset.size(); ++i) {
            global_offset[i] = jglobal_offset_arr[i].as_double();
        }

        edm::coord::CoordinateManager::map_t coord_map;
        for (std::size_t i = 0; i < jcoord_offsets_arr.size(); ++i) {
            auto c = jcoord_offsets_arr.at(i).as<edm::coord::Coordinate>();
            if (coord_map.find(c.index()) != coord_map.end()) {
                return false;
            }

            coord_map.emplace(c.index(), c);
        }

        t.clear_coordinates_map();
        t.set_global_offset(global_offset);
        t.set_coordinates_map(std::move(coord_map));
        
        return true;
    }
};
} // namespace json::ext

static std::optional<std::string>
_create_backup_file_for(const std::string ori_file) {
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

bool CoordinateManager::save_as(const std::string &filename) const {
    return _save_to_file(filename);
}

std::optional<CoordinateManager> CoordinateManager::LoadFromJsonFile(const std::string& filename)
{
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

    return root.as<CoordinateManager>();
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
