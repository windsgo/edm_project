#include "PointMoveHandler.h"

#include "Motion/MotionUtils/MotionUtils.h"
#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

namespace edm {

namespace move {

PointMoveHandler::PointMoveHandler() { clear(); }

bool PointMoveHandler::start(const MoveRuntimePlanSpeedInput &speed_param,
                             const axis_t &start_pos,
                             const axis_t &target_pos) {
    start_pos_ = start_pos;
    target_pos_ = target_pos;
    curr_pos_ = start_pos_;

    MotionUtils::CalcAxisUnitVector(start_pos, target_pos, unit_vector_);

    target_length_ = MotionUtils::CalcAxisLength(start_pos, target_pos);
    curr_length_ = 0.0;

    // s_logger->debug("sp: {}, tp: {}, uv:{}, tl: {}", start_pos_[0], target_pos_[0], unit_vector_[0], target_length_);

    return mrt_wrapper_.start(speed_param, target_length_);
}

bool PointMoveHandler::pause() { return mrt_wrapper_.pause(); }

bool PointMoveHandler::resume() { return mrt_wrapper_.resume(); }

bool PointMoveHandler::stop(bool immediate) {
    return mrt_wrapper_.stop(immediate);
}

bool PointMoveHandler::change_speed(unit_t new_speed) {
    return mrt_wrapper_.change_speed(new_speed);
}

void PointMoveHandler::clear() {
    mrt_wrapper_.clear();
    // 数据成员没有必要重置了, 外层需要知道自己在做什么
    // 以及数据成员是否有效

    // 重置几个标量
    // curr_length_ = 0.0;
    // target_length_ = 0.0;
}

void PointMoveHandler::run_once() {
    mrt_wrapper_.run_once();

    if (!mrt_wrapper_.is_started()) {
        s_logger->warn("{}, not started.", __FUNCTION__);
        return;
    }

    if (mrt_wrapper_.is_over()) {
        curr_length_ = target_length_;
        curr_pos_ = target_pos_;
        return;
    }

    curr_length_ = mrt_wrapper_.get_current_length();

    // curr_pos_
    auto temp = MotionUtils::ScaleAxis(unit_vector_, curr_length_);
    for (int i = 0; i < curr_pos_.size(); ++i) {
        curr_pos_[i] = start_pos_[i] + temp[i];
    }
}

} // namespace move

} // namespace edm
