#include "Logger/LogMacro.h"

#include <fmt/color.h>
#include <fmt/format.h>

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

#include "Motion/Trajectory/TrajectoryList.h"

using namespace edm::move;

TrajectoryList::ptr trajectory_list;

static void init() {
    TrajectoryList::container_type init_container;

    axis_t a1 = {0};
    axis_t a2 = {1};

    auto s1 = std::make_shared<TrajectoryLinearSegement>(a1, a2);
    init_container.push_back(s1);

    a1 = {1};
    a2 = {3};
    auto s2 = std::make_shared<TrajectoryLinearSegement>(a1, a2);
    init_container.push_back(s2);

    a1 = {3};
    a2 = {-5};
    auto s3 = std::make_shared<TrajectoryLinearSegement>(a1, a2);
    init_container.push_back(s3);

    trajectory_list =
        std::make_shared<TrajectoryList>(std::move(init_container));
}

static void print_status(int i, unit_t inc, TrajectoryList::ptr tl) {
    if (inc > 0)
        s_logger->debug(
            "i = {:03d}, inc = {: 02.4f}: size: {}, idx: {}, "
            "axis[0]: {: 02.4f}; as: {}, ae: {}; type: {}",
            i, fmt::styled(inc, fmt::fg(fmt::color::green)), tl->size(),
            tl->curr_segement_index(), tl->get_curr_cmd_axis()[0],
            (int)tl->at_start_segement(), (int)tl->at_end_segement(),
            (int)tl->get_curr_segement()->type());

    else
        s_logger->debug(
            "i = {:03d}, inc = {: 02.4f}: size: {}, idx: {}, "
            "axis[0]: {: 02.4f}; as: {}, ae: {}; type: {}",
            i, fmt::styled(inc, fmt::fg(fmt::color::red)), tl->size(),
            tl->curr_segement_index(), tl->get_curr_cmd_axis()[0],
            (int)tl->at_start_segement(), (int)tl->at_end_segement(),
            (int)tl->get_curr_segement()->type());
}

static void test_trajectory() {
    init();

    unit_t inc = 0.3;

    for (int i = 0; i < 500; ++i) {
        std::srand(i * 10 * std::time(nullptr));
        inc = (double)(std::rand() % 100) / 100 - 0.4;
        trajectory_list->run_once(inc);
        print_status(i, inc, trajectory_list);

        if (trajectory_list->at_end_segement() &&
            trajectory_list->get_curr_segement()->at_end()) {
            break;
        }
    }
}

int main() {

    test_trajectory();

    return 0;
}