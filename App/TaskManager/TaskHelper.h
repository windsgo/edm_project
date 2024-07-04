#pragma once

#include <QCoreApplication>
#include <QDateTime>
#include <optional>
#include <string>

#include "Coordinate/Coordinate.h"
#include "Coordinate/CoordinateSystem.h"
#include "Motion/MotionThread/MotionCommand.h"
#include "Motion/MoveDefines.h"
#include "SystemSettings/SystemSettings.h"
#include "Utils/Format/edm_format.h"
#include "Utils/UnitConverter/UnitConverter.h"

namespace edm {

namespace task {

class TaskHelper final {
public:
    static bool WaitforCmdTobeAccepted(move::MotionCommandBase::ptr cmd,
                                       int timeout_ms = -1) {
        auto start_t = QDateTime::currentMSecsSinceEpoch();

        while (1) {
            if (cmd->is_accepted() || cmd->is_ignored()) {
                break;
            }

            auto now = QDateTime::currentMSecsSinceEpoch();
            if (timeout_ms >= 0 && now - start_t > timeout_ms) {
                // timeout_ms < 0 means wait forever
                break;
            }

            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }

        return cmd->is_accepted();
    }

    static bool CheckPosandnegSoftLimit(coord::CoordinateSystem::ptr coord_sys,
                                        const move::axis_t &mach_pos, const move::axis_t& dir) {
        const auto &pos_sl = coord_sys->get_pos_soft_limit();
        const auto &neg_sl = coord_sys->get_neg_soft_limit();

        for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
            if (dir[i] == 0.0) {
                continue;
            }

            if (dir[i] > 0.0 && mach_pos[i] > pos_sl[i]) {
                return false;
            } else if (dir[i] < 0.0 && mach_pos[i] < neg_sl[i]) {
                return false;
            }
        }

        return true;
    }

    static move::MoveRuntimePlanSpeedInput GetDefaultSpeedparam() {
        edm::move::MoveRuntimePlanSpeedInput speed;

        speed.nacc = util::UnitConverter::ms2p(
            SystemSettings::instance().get_fmparam_nacc_ms());
        speed.acc0 = util::UnitConverter::um2blu(
            SystemSettings::instance().get_fmparam_max_acc_um_s2());
        speed.dec0 = -speed.acc0;
        speed.entry_v = 0;
        speed.exit_v = 0;
        speed.cruise_v =
            1000; // default, modify outside according to feed speed

        return speed;
    }

    // 计算给定目标方向，从reference_pos计算，可以运动的最大距离
    // dir 需要是一个单位向量
    static move::unit_t
    GetMaxLengthOnCurrentDir(coord::CoordinateSystem::ptr coord_sys,
                             const move::axis_t &dir,
                             const move::axis_t &reference_pos) {
        const auto &pos_sl = coord_sys->get_pos_soft_limit();
        const auto &neg_sl = coord_sys->get_neg_soft_limit();

        double scale{0.0}; // 倍率

        for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
            double left_length;
            double curr_axes_left_scale;
            if (dir[i] > 0) {
                left_length = pos_sl[i] - reference_pos[i]; // 正方向剩余距离
                if (left_length <= 0) {
                    return 0; // 有运动轴已经在正限位
                }
            } else if (dir[i] < 0) {
                left_length = neg_sl[i] - reference_pos[i]; // 正方向剩余距离
                if (left_length >= 0) {
                    return 0; // 有运动轴已经在负限位
                }
            } else {
                // 该方向无运动, 不计算
                continue;
            }

            // 该方向剩余的倍率
            curr_axes_left_scale = left_length / dir[i];
            if (scale == 0 || curr_axes_left_scale < scale) {
                scale = curr_axes_left_scale; // scale取更小的那一个
            }
            continue;
        }

        return scale * move::MotionUtils::CalcAxisLength(dir);
    }
};

} // namespace task

} // namespace edm
