#include "TrajectoryList.h"

#include "Logger/LogMacro.h"

#include "Exception/exception.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {

namespace move {

void TrajectoryList::run_once(unit_t inc) {
    assert(!empty());
    if (empty()) {
        s_logger->error("{}: running empty TrajectoryList",
                        __PRETTY_FUNCTION__);
        return;
    }

    (*curr_segement_iter_)->run_once(inc);

    if (inc > 0) {
        if ((*curr_segement_const_iter_)->at_end() && !at_end_segement()) {
            _add_iter();
            (*curr_segement_iter_)->set_at_start();
        }
    } else if (inc < 0) {
        if ((*curr_segement_const_iter_)->at_start() && !at_start_segement()) {
            _dec_iter();
            (*curr_segement_iter_)->set_at_end();
        }
    }
}

void TrajectoryList::_assert_not_empty_or_throw() {
    assert(!empty());
    bool member_has_nullptr = false;
    for (const auto &i : segements_) {
        if (i == nullptr) {
            member_has_nullptr = true;
            break;
        }
    }

    if (empty() || member_has_nullptr) {
        s_logger->critical("empty trajectory or member_has_nullptr, size: {}",
                           segements_.size());
        throw exception("empty trajectory or member_has_nullptr");
    }
}

} // namespace move

} // namespace edm
