#pragma once

#include <stack>

#include "Motion/MoveDefines.h"
#include "Motion/PointMoveHandler/PointMoveHandler.h"

namespace edm
{

namespace move
{

struct AxisRecorderData {
    axis_t start_point_;
    axis_t stop_point_;
    MoveRuntimePlanSpeedInput plan_param_;
};

using AxisRecordStack = std::stack<AxisRecorderData>;

template <typename T>
void ClearStackOrQueue(T& v) {
    T{}.swap(v);
}

// class AxisRecorder final {
// public:
//     using ptr = std::shared_ptr<AxisRecorder>;
//     AxisRecorder() noexcept = default;
//     ~AxisRecorder() noexcept = default;

//     void clear();

//     void push(const AxisRecorderData& data);
//     void push(AxisRecorderData&& data);

//     bool empty() const;

// private:
//     std::stack<AxisRecorderData> recorded_data_;
// };

} // namespace move

} // namespace edm


