#include "Coordinate/CoordinateManager.h"

#include <iostream>
#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

#include "config.h"

using namespace edm::coord;
using namespace edm::move;

int main () {

    CoordinateManager cm(EDM_ROOT_DIR "/Conf/coord.json");

    const auto& d = cm.data();

    s_logger->debug("filename: {}", cm.get_filename());
    for (const auto& [i, c] : d) {
        s_logger->debug("{}: {}, {}", i, c.index(), c.offset()[0]);
    } 

    cm.save_as(EDM_CONFIG_DIR  "coord6.json");

    auto ccc = std::make_shared<CoordinateManager>(EDM_CONFIG_DIR "coord6.json");

    return 0;
}