#pragma once

#include <deque>
#include <list>
#include <memory>

#include "Motion/MotionUtils/MotionUtils.h"
#include "Motion/MoveDefines.h"
#include "TrajectorySegement.h"

namespace edm {

namespace move {

class TrajectoryList {
public:
    using ptr = std::shared_ptr<TrajectoryList>;
    using element_type = TrajectorySegementBase::ptr;
    using container_type = std::deque<element_type>;
    using iterator_type = container_type::iterator;
    TrajectoryList() noexcept = default;
    ~TrajectoryList() noexcept = default;

    inline bool empty() const { return segements_.empty(); }
    inline auto size() const { return segements_.size(); }

    

private:
    //! 使用迭代器表示当前段, 为了防止迭代器失效,
    //! 不提供修改segements_容器的方法, 重新生成段必须重新构造
    container_type segements_;
    iterator_type curr_segement_;
};

} // namespace move

} // namespace edm
