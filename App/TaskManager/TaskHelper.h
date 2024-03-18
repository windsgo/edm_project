#pragma once

#include <QCoreApplication>
#include <QDateTime>

#include "Coordinate/CoordinateSystem.h"
#include "Motion/MotionThread/MotionCommand.h"
#include "SystemSettings/SystemSettings.h"
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
                                        const move::axis_t &mach_pos) {
        const auto &pos_sl = coord_sys->get_pos_soft_limit();
        const auto &neg_sl = coord_sys->get_neg_soft_limit();

        for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
            if (mach_pos[i] > pos_sl[i] || mach_pos[i] < neg_sl[i]) {
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
};

} // namespace task

} // namespace edm
