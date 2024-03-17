#pragma once

// 定义GCode命令Base类 (从json生成为GCode命令, 然后组成vector,
// 传递给TaskManager)

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
    PauseCommand = 98,          // m00 //! 先忽略, 后面可以给motion发送一个假的auto命令, 在Motion那边直接进入Auto-Paused状态
    ProgramEndCommand = 99,     // m02 //! 忽略 或 退出都可
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
    
private:
    GCodeTaskType type_;

    int line_number_; // 文件行号
    int node_index_; // TODO node编号(后续用tree节点制作node组显示)

protected:
};

} // namespace task

} // namespace edm
