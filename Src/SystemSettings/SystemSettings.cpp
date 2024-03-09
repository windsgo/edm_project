#include "SystemSettings.h"

namespace edm
{

static auto dummy_get_init = SystemSettings::instance();

bool SystemSettings::_save_to_file(const std::string &filename) const {
    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        return false;
    }

    json::value jv {data_};
    ofs << jv;

    ofs.close();
    return true;
}

} // namespace edm
