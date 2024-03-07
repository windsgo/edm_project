#include "TrajectorySegement.h"

#include <cassert>

namespace edm {

namespace move {

void TrajectoryLinearSegement::set_at_start() {
    curr_length_ = 0;
    curr_pos_ = start_pos_;
}

void TrajectoryLinearSegement::set_at_end() {
    curr_length_ = total_length_;
    curr_pos_ = end_pos_;
}

bool TrajectoryLinearSegement::at_start() const { return curr_length_ <= 0.0; }

bool TrajectoryLinearSegement::at_end() const {
    return curr_length_ >= total_length_;
}

void TrajectoryLinearSegement::run_once(unit_t inc) {
    _add_inc(inc);

    curr_pos_ = CalcCurrPos(start_pos_, end_pos_, curr_length_);
}

void TrajectoryLinearSegement::_add_inc(unit_t inc) {
    curr_length_ = curr_length_ + inc;

    _validate_curr_length();
}

inline void TrajectoryLinearSegement::_validate_curr_length() {
    if (curr_length_ > total_length_) {
        curr_length_ = total_length_;
        assert(at_end());
    } else if (curr_length_ <= 0.0) {
        curr_length_ = 0.0;
        assert(at_start());
    }
}

axis_t edm::move::TrajectoryLinearSegement::CalcCurrPos(const axis_t &start_pos,
                                                        const axis_t &end_pos,
                                                        unit_t curr_length) {
    axis_t ret = MotionUtils::ScaleAxis(
        MotionUtils::CalcAxisUnitVector(start_pos, end_pos), curr_length);

    for (int i = 0; i < ret.size(); ++i) {
        ret[i] += start_pos[i];
    }

    return ret; // NRVO
}

} // namespace move

} // namespace edm
