#pragma once

#include <string>
#include <optional>
#include <vector>

namespace edm {

namespace util {

struct netdev_info {
    std::string name;
    bool link_up = false;
};

std::optional<std::vector<netdev_info>> get_netdev_list();

std::optional<netdev_info> get_netdev_info(const std::string &netdev_name);

bool is_netdev_exist(const std::string &netdev_name);

} // namespace util

} // namespace edm
