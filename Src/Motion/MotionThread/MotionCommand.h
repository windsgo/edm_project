#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

#include "Motion/MotionUtils/MotionUtils.h"
#include "Motion/MoveDefines.h"
#include "Motion/JumpDefines.h"
#include "Motion/MoveruntimeWrapper/MoveruntimeWrapper.h"

#include "Exception/exception.h"
#include "Utils/Format/edm_format.h"

namespace edm {

namespace move {

//! Motion Command:
//! 指直接发送给 MotionCommandQueue 队列的命令
//! 最后由MotionThread线程取出命令, 并操作Motion状态机进行执行/忽略

enum MotionCommandType {
    MotionCommand_Unknown = 0x0,

    //! 区分Manual命令和Auto命令, 还有运动无关的Setting命令
    //! Setting命令包括:
    //! 抬刀参数、各轴软限位(抬刀计算使用)、其他参数的设定(或更新)
    //! Manual命令包括: 无运动时的界面(手控盒)点动, 暂停时点动(自由,
    //! Nurbs原轨迹), 火花定位时点动等 Auto命令包括: G00,
    //! G01等其他G代码运动、火花碰边运动的启动; Auto模式统一的Pause, Resume,
    //! Stop命令

    /* Manual 命令 */

    // 手动点动/暂停点动/火花碰边点动统一接口 命令
    MotionCommandManual_StartPointMove,
    MotionCommandManual_StopPointMove,

    // 紧急停止所有运动
    MotionCommandManual_EmergencyStopAllMove,

    /* Auto 命令 */

    // 启动G00快速移动 命令
    MotionCommandAuto_StartG00FastMove,

    // 启动G01伺服加工 命令
    MotionCommandAuto_StartG01ServoMove,

    // 火花碰边 命令
    MotionCommandAuto_StartManualTouchMove, // TODO

    // Auto 运行模式下统一的暂停, 继续, 停止命令 (执行模块会根据不同状态执行)
    // 包括G00, G01, 火花碰边定位 的 暂停, 继续, 停止
    MotionCommandAuto_Pause,
    MotionCommandAuto_Resume,
    MotionCommandAuto_Stop,

    // 这个命令实现M00暂停
    MotionCommandAuto_M00FakePauseTask,

    // 延时
    MotionCommandAuto_G04Delay,

    /* Setting 命令 */

    // 设定/更改抬刀参数
    MotionCommandSetting_SetJumpParam, // TODO

    // 触发Ecat连接尝试
    MotionCommandSetting_TriggerEcatConnect,

    // 清除报警
    MotionCommandSetting_ClearWarning,

    MotionCommand_Max
};

// Motion命令基类
class MotionCommandBase {
public:
    using ptr = std::shared_ptr<MotionCommandBase>;
    MotionCommandBase() = delete;
    MotionCommandBase(MotionCommandType type) : type_(type) {
        if (type > MotionCommand_Max || type == MotionCommand_Unknown) {
            throw exception(EDM_FMT::format(
                "MotionCommandBase unknown type: {}", static_cast<int>(type)));
        }
    }
    virtual ~MotionCommandBase() noexcept = default;

    MotionCommandType type() const { return type_; }

    //! accept, ignore 由执行者调用, 用于表明执行者对命令输入的处理结果
    //! accept表示已接受并将要执行, ignore表示不接受
    //!! 需要注意的是,
    //!!
    //! 命令的生产者在发布命令后只能调用is_accepted()和is_ignored()来获取命令的执行状态
    //!! 发布者调用其他任何函数都不保证数据安全, 可能产生未定义行为
    //! accepted ignored 只是一个参考返回值,
    //! 发布者最好还是要依赖定时获取的状态结构体
    void accept() { accepted_state_ = 1; }
    void ignore() { accepted_state_ = 2; }
    bool is_accepted() const { return accepted_state_ == 1; }
    bool is_ignored() const { return accepted_state_ == 2; }

private:
    MotionCommandType type_;

    // accepted_state_: 0: 初始化, 1: accepted, 2: ignored
    std::atomic_uint8_t accepted_state_{0};
};

class MotionCommandManualEmergencyStopAllMove : public MotionCommandBase {
public:
    MotionCommandManualEmergencyStopAllMove()
        : MotionCommandBase(MotionCommandManual_EmergencyStopAllMove) {}
    ~MotionCommandManualEmergencyStopAllMove() noexcept override = default;
};
class MotionCommandAutoM00FakePauseTask : public MotionCommandBase {
public:
    MotionCommandAutoM00FakePauseTask()
        : MotionCommandBase(MotionCommandAuto_M00FakePauseTask) {}
    ~MotionCommandAutoM00FakePauseTask() noexcept override = default;
};

class MotionCommandAutoG04Delay : public MotionCommandBase {
public:
    MotionCommandAutoG04Delay(double delay_s)
        : MotionCommandBase(MotionCommandAuto_G04Delay), delay_s_(delay_s) {}
    ~MotionCommandAutoG04Delay() noexcept override = default;

    auto delay_s() const { return delay_s_; }

private:
    double delay_s_;
};

// Motion简单直线(广义长度)运动基类 (提供数据成员 起点, 终点,
// 并计算直线长度(对于广义直线, 是2范数))
class MotionCommandLinearMoveBase : public MotionCommandBase {
public:
    MotionCommandLinearMoveBase(MotionCommandType type, const axis_t &start,
                                const axis_t &end)
        : MotionCommandBase(type), start_pos_(start), end_pos_(end) {
        length_ = MotionUtils::CalcAxisLength(
            MotionUtils::CalcAxisVector(start_pos_, end_pos_));
    }
    virtual ~MotionCommandLinearMoveBase() noexcept override = default;

    const axis_t &start_pos() const { return start_pos_; }
    const axis_t &end_pos() const { return end_pos_; }
    unit_t length() const { return length_; }

protected:
    axis_t start_pos_;
    axis_t end_pos_;
    unit_t length_; // 长度 (2-范数)
};

// 启动手动直线点动
class MotionCommandManualStartPointMove final
    : public MotionCommandLinearMoveBase {
public:
    MotionCommandManualStartPointMove(
        const axis_t &start, const axis_t &end,
        const MoveRuntimePlanSpeedInput &speed_param,
        bool touch_detect_enable = true)
        : MotionCommandLinearMoveBase(MotionCommandManual_StartPointMove, start,
                                      end),
          speed_param_(speed_param), touch_detect_enable_(touch_detect_enable) {
    }
    ~MotionCommandManualStartPointMove() noexcept override = default;

    inline const auto &speed_param() const { return speed_param_; };

    inline bool touch_detect_enable() const { return touch_detect_enable_; }

private:
    MoveRuntimePlanSpeedInput speed_param_;
    bool touch_detect_enable_;
};

// 停止手动直线点动
class MotionCommandManualStopPointMove final : public MotionCommandBase {
public:
    MotionCommandManualStopPointMove(bool immediate_stop = false)
        : MotionCommandBase(MotionCommandManual_StopPointMove),
          immediate_(immediate_stop) {}
    ~MotionCommandManualStopPointMove() noexcept override = default;

    bool immediate() const { return immediate_; }

private:
    bool immediate_;
};

// 触发Ecat连接
class MotionCommandSettingTriggerEcatConnect final : public MotionCommandBase {
public:
    MotionCommandSettingTriggerEcatConnect()
        : MotionCommandBase(MotionCommandSetting_TriggerEcatConnect) {}
    ~MotionCommandSettingTriggerEcatConnect() noexcept override = default;
};

class MotionCommandSettingClearWarning final : public MotionCommandBase {
public:
    MotionCommandSettingClearWarning(int flag [[maybe_unused]])
        : MotionCommandBase(MotionCommandSetting_ClearWarning) {}
    ~MotionCommandSettingClearWarning() noexcept override = default;
};

class MotionCommandSettingSetJumpParam final : public MotionCommandBase {
public:
    MotionCommandSettingSetJumpParam(const JumpParam &jump_param)
        : MotionCommandBase(MotionCommandSetting_SetJumpParam),
          jump_param_(jump_param) {}
    ~MotionCommandSettingSetJumpParam() noexcept override = default;

    const auto &jump_param() const { return jump_param_; }

private:
    move::JumpParam jump_param_;
};

// 启动Auto G00快速移动
class MotionCommandAutoStartG00FastMove final
    : public MotionCommandLinearMoveBase {
public:
    MotionCommandAutoStartG00FastMove(
        const axis_t &start, const axis_t &end,
        const MoveRuntimePlanSpeedInput &speed_param,
        bool touch_detect_enable = true)
        : MotionCommandLinearMoveBase(MotionCommandAuto_StartG00FastMove, start,
                                      end),
          speed_param_(speed_param), touch_detect_enable_(touch_detect_enable) {
    }
    ~MotionCommandAutoStartG00FastMove() noexcept override = default;

    inline const auto &speed_param() const { return speed_param_; };

    inline bool touch_detect_enable() const { return touch_detect_enable_; }

private:
    MoveRuntimePlanSpeedInput speed_param_;
    bool touch_detect_enable_;
};

// 启动Auto G00快速移动
class MotionCommandAutoStartG01ServoMove final
    : public MotionCommandLinearMoveBase {
public:
    MotionCommandAutoStartG01ServoMove(const axis_t &start, const axis_t &end,
                                       unit_t max_jump_height_from_begin)
        : MotionCommandLinearMoveBase(MotionCommandAuto_StartG01ServoMove,
                                      start, end),
          max_jump_height_from_begin_(max_jump_height_from_begin) {}
    ~MotionCommandAutoStartG01ServoMove() noexcept override = default;

    auto max_jump_height_from_begin() const {
        return max_jump_height_from_begin_;
    }

private:
    unit_t max_jump_height_from_begin_;
};

class MotionCommandAutoStop final : public MotionCommandBase {
public:
    MotionCommandAutoStop(bool immediate = false)
        : MotionCommandBase(MotionCommandAuto_Stop), immediate_(immediate) {}
    ~MotionCommandAutoStop() noexcept override = default;

    inline bool immediate() const { return immediate_; }

private:
    bool immediate_;
};

class MotionCommandAutoPause final : public MotionCommandBase {
public:
    MotionCommandAutoPause() : MotionCommandBase(MotionCommandAuto_Pause) {}
    ~MotionCommandAutoPause() noexcept override = default;
};

class MotionCommandAutoResume final : public MotionCommandBase {
public:
    MotionCommandAutoResume() : MotionCommandBase(MotionCommandAuto_Resume) {}
    ~MotionCommandAutoResume() noexcept override = default;
};

// class MotionCommandStartLinearServoMove final
//     : public MotionCommandSimpleMoveBase {
// public:
//     MotionCommandStartLinearServoMove(const axis_t &start, const axis_t &end)
//         : MotionCommandSimpleMoveBase(MotionCommandType_StartLinearServoMove,
//                                       start, end) {}
//     ~MotionCommandStartLinearServoMove() noexcept override = default;
// };

} // namespace move

} // namespace edm
