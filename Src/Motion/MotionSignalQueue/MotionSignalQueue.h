#pragma once

// 用于从MotionThread中传递出信号, 信号必须轻量
// 信号一般是运动已开始, 已暂停, 已继续, 已结束这类
// 外层控制层负责从队列中取出元素, 并将其转换为qt信号,
// 最后告知到任务控制类、GUI类等

// 可以考虑使用boost::lockfree::queue / spsc_queue(单生产者, 单消费者)

#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp> // 好像会有问题, empty()无法调用

#include "Motion/MoveDefines.h"
#include "config.h"

#include <memory>

namespace edm {

namespace move {

//! 直接用boost的lockfree::queue
//! 注意接口与标准接口不同, 主要使用push, consume_one
//! empty() 方法是不可靠的, 因此可能要在consume_one里面设定标志位,
//! 来判断是否consume了一个信号
// using MotionSignalQueue =
//     boost::lockfree::queue<MotionSignal, boost::lockfree::capacity<1024>,
//                            boost::lockfree::fixed_sized<false>>;

// 包装一下操作, 因为queue和spsc_queue的操作不同
class MotionSignalQueue final {
public:
    using ptr = std::shared_ptr<MotionSignalQueue>;
#ifdef EDM_MOTION_SIGNAL_QUEUE_USE_SPSC
    //! 效率更高, 不可动态扩容, 仅限于 "单个生产者, 单个消费者"
    using queue_t =
        boost::lockfree::spsc_queue<MotionSignal,
                                    boost::lockfree::capacity<1024>,
                                    boost::lockfree::fixed_sized<true>>;
#else  // EDM_MOTION_SIGNAL_QUEUE_USE_SPSC
    //! 可动态扩容
    using queue_t =
        boost::lockfree::queue<MotionSignal, boost::lockfree::capacity<1024>,
                               boost::lockfree::fixed_sized<false>>;
#endif // EDM_MOTION_SIGNAL_QUEUE_USE_SPSC

public:
    inline bool push(const MotionSignal &signal) { return queue_.push(signal); }

    template <typename Func> inline bool consume_one(Func &&f) {
        return queue_.consume_one(std::forward<Func>(f));
    }

    template <typename Func> inline std::size_t consume_all(Func &&f) {
        return queue_.consume_all(std::forward<Func>(f));
    }

    template <typename U>
    inline bool pop(U& p) {
        return queue_.pop<U>(p);
    }

    // 不提供 empty(), size() 等访问容量的方法
    // boost::lockfree::queue 的 empty() 方法不安全, 且没有任何 size() 方法
    // boost::lockfree::spsc_queue 只支持消费者访问 read_avaiable()
    //! 消费者实现只能依赖 consume_one 或 consume_all 的返回值

    //! unsafe
    queue_t& raw_queue() { return queue_; }

private:
    queue_t queue_;
};

} // namespace move

} // namespace edm
