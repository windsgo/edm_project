#pragma once

// 定义GCode命令Base类 (从json生成为GCode命令, 然后组成vector,
// 传递给TaskManager)

#include <chrono>
#include <cstdint>
#include <memory>

#include <json.hpp>

namespace edm {

namespace task {

//! 与 Python 中定义同步

enum class GCodeCoordinateMode {
    Undefined = -1,
    AbsoluteMode = 0,  // G90
    IncrementMode = 1, // G91
};

//! 忽略的: G90/G91, f(1000), 
//! 在外层做的: e(100), m02(停止), g54(坐标系切换)
//! 发给MotionThread的: g00, g01, m00(下发一个以开始就暂停的空任务), delay(延时, 放在下面去做)
enum class GCodeTaskType {
    Undefined = -1,
    G00MotionCommand = 0,       // g00
    G01MotionCommand = 1,       // g01
    CoordinateModeCommand = 2,  // g90 g91 //! 忽略
    CoordinateIndexCommand = 3, // g54, g55, etc.
    EleparamSetCommand = 4,     // e001
    FeedSpeedSetCommand = 5,    // f1000 //! 忽略
    DelayCommand = 6,           // g04t5
    CoordSetZeroCommand = 7,    // coord_set_x_zero, coord_set_zero(x=True, z=True), coord_set_all_zero(), etc
    DrillMotionCommand = 8,     // 打孔指令
    G01GroupMotionCommand = 9,  // g01组合指令
    PauseCommand = 98,          // m00 //! 先忽略, 后面可以给motion发送一个假的auto命令, 在Motion那边直接进入Auto-Paused状态
    ProgramEndCommand = 99,     // m02 //! 忽略 或 退出都可
};

class TaskTimer {
public:
    TaskTimer() { reset(); }
    ~TaskTimer() = default;

    enum class State {
        Idle,
        Running,
        Paused,
        Stopped
    };

    void reset() {
        elapsed_time_buffer_ = std::chrono::system_clock::duration::zero();
        state_ = State::Idle;
    }

    void restart() {
        reset();
        start_time_ = std::chrono::system_clock::now();
        total_start_time_ = start_time_;
        state_ = State::Running;
    }

    void stop() {
        if (state_ == State::Running) {
            end_time_ = std::chrono::system_clock::now();
            state_ = State::Stopped;
            elapsed_time_buffer_ += (end_time_ - start_time_);
        } else if (state_ == State::Paused) {
            end_time_ = std::chrono::system_clock::now();
            state_ = State::Stopped;
        } else if (state_ == State::Idle) {
            end_time_ = std::chrono::system_clock::now();
            start_time_ = end_time_;
            total_start_time_ = start_time_;
            state_ = State::Stopped;
        }
    }

    void pause() {
        if (state_ == State::Running) {
            auto now = std::chrono::system_clock::now();
            elapsed_time_buffer_ += (now - start_time_);
            state_ = State::Paused;
        }
    }

    void resume() {
        if (state_ == State::Paused) {
            start_time_ = std::chrono::system_clock::now();
            state_ = State::Running;
        }
    }

    std::chrono::system_clock::duration elapsed_time() const {
        if (state_ == State::Running) {
            auto now = std::chrono::system_clock::now();
            return elapsed_time_buffer_ + (now - start_time_);
        } else if (state_ == State::Paused || state_ == State::Stopped) {
            return elapsed_time_buffer_;
        } else {
            return std::chrono::system_clock::duration::zero();
        }
    }

    const auto& start_time() const { return total_start_time_; }
    const auto& end_time() const { return end_time_; }

    auto state() const { return state_; }
    auto is_running() const { return state_ == State::Running; }

private:
    State state_{State::Idle};
    std::chrono::system_clock::time_point total_start_time_; // 总的开始时间
    std::chrono::system_clock::time_point start_time_; // 暂停继续后的开始时间
    std::chrono::system_clock::time_point end_time_;
    
    std::chrono::system_clock::duration elapsed_time_buffer_; // 暂停的时候, 把已经过去的时间累积起来
};

class GCodeTaskBase {
public:
    using ptr = std::shared_ptr<GCodeTaskBase>;
    GCodeTaskBase(GCodeTaskType type, int line_number, int node_index = -1)
        : type_(type), line_number_(line_number), node_index_(node_index) {}
    virtual ~GCodeTaskBase() noexcept = default;

    auto type() const { return type_; }

    auto line_number() const { return line_number_; }
    auto node_index() const { return node_index_; }

    virtual bool is_motion_task() const = 0;

    void set_gcode_str(const std::string &gcode_str) {
        gcode_str_ = gcode_str;
    }
    const auto &get_gcode_str() const { return gcode_str_; }
    
    auto& timer() { return timer_; }
    const auto& timer() const { return timer_; }

    void set_options(const std::vector<std::string> &options) {
        options_ = options;
    }
    const auto &get_options() const { return options_; }
    
private:
    GCodeTaskType type_;

    int line_number_; // 文件行号
    int node_index_; // TODO node编号(后续用tree节点制作node组显示)

    // 用于记录时间等信息
    std::string gcode_str_; // 代码记录
    TaskTimer timer_; // 计时器

    // 后续很多都可以用这种options带字符串参数的方式快速传参, 便于开发...
    std::vector<std::string> options_;

protected:
};

} // namespace task

} // namespace edm
