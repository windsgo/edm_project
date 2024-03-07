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
    using element_type = TrajectorySegementBase::ptr;
    using container_type = std::deque<element_type>;
    using iterator_type = container_type::iterator;
    using const_iterator_type = container_type::const_iterator;

public:
    using ptr = std::shared_ptr<TrajectoryList>;
    TrajectoryList(const container_type &segements)
        : segements_(segements), curr_segement_iter_(segements_.begin()),
          curr_segement_const_iter_(segements_.cbegin()) {
        _assert_not_empty_or_throw();
    }
    TrajectoryList(container_type &&segements)
        : segements_(std::move(segements)),
          curr_segement_iter_(segements_.begin()),
          curr_segement_const_iter_(segements_.cbegin()) {
        _assert_not_empty_or_throw();
    }

    ~TrajectoryList() noexcept = default;

    inline bool empty() const {
        return segements_.empty();
    } // should not be empty
    inline auto size() const { return segements_.size(); }
    inline bool at_end_segement() const {
        return curr_segement_const_iter_ == segements_.end() - 1;
    }
    inline bool at_start_segement() const {
        return curr_segement_const_iter_ == segements_.begin();
    }
    inline bool at_end() const {
        return at_end_segement() && (*curr_segement_const_iter_)->at_end();
    }
    inline bool at_start() const {
        return at_start_segement() && (*curr_segement_const_iter_)->at_start();
    }

    // for special segement operation or information fetch
    element_type get_curr_segement() const;
    inline TrajectorySegementType get_curr_segement_type() const {
        return get_curr_segement()->type();
    }

    bool get_curr_cmd_axis(axis_t &axis) const;
    const axis_t &get_curr_cmd_axis() const;

    inline std::size_t curr_segement_index() const {
        // std::distance is O(1) to std::deque's `LegacyRandomAccessIterator`
        return std::distance(segements_.begin(), curr_segement_const_iter_);
    }

    void run_once(unit_t inc);

private:
    void _assert_not_empty_or_throw();

    inline void _add_iter() { ++curr_segement_iter_; ++curr_segement_const_iter_; }
    inline void _dec_iter() { --curr_segement_iter_; --curr_segement_const_iter_; }

private:
    //! 使用迭代器表示当前段, 为了防止迭代器失效,
    //! 不提供修改segements_容器的方法, 重新生成段必须重新构造
    container_type segements_;
    iterator_type curr_segement_iter_;
    const_iterator_type curr_segement_const_iter_;
};

} // namespace move

} // namespace edm
