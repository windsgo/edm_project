#pragma once

#include "Motion/MoveDefines.h"

#include <functional>
#include <memory>

namespace edm
{

namespace move
{

class TouchDetectHandler final {
public:
    using ptr = std::shared_ptr<TouchDetectHandler>;
    TouchDetectHandler(const std::function<bool(void)> physical_touch_detect_cb);
    ~TouchDetectHandler() noexcept = default;

    void reset();

    inline void clear_warning() { warning_ = false; }
    inline bool has_warning() const { return warning_; }

    inline void set_detect_enable(bool enable) { detect_enabled_ = enable; }
    inline bool is_detect_enable() const { return detect_enabled_; }

    inline bool physical_detected() const { return physical_touch_detect_cb_(); }

    // 进行一次检测, 如果触发warning, 返回true, 否则返回false
    bool run_detect_once();

private:
    std::function<bool(void)> physical_touch_detect_cb_;

    bool detect_enabled_ {false};
    bool warning_ {false};
};
    
} // namespace move

} // namespace edm
