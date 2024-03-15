#pragma once

#include <QObject>
#include <QTimer>

#include <optional>

#include "QtDependComponents/CanController/CanController.h"
#include "QtDependComponents/IOController/IOController.h"
#include "QtDependComponents/PowerController/PowerController.h"

#include "PowerDatabase.h"

namespace edm {
namespace app {

class PowerManager final : public QObject
{
    Q_OBJECT
public:
    explicit PowerManager(io::IOController::ptr io_ctrler, power::PowerController::ptr power_ctrler, uint32_t cycle_ms = 1000, QObject *parent = nullptr);
    ~PowerManager();

    const auto& get_db() const { return *power_db_; }

    // 设置操作, 新的eleparam
    bool set_current_eleparam_index(uint32_t index);
    bool set_current_eleparam(const edm::power::EleParam_dkd_t& eleparam);

    // 转发操作数据库
    inline bool exist_eleparam_index(uint32_t index) const { return power_db_->exist_index(index); }
    inline bool get_eleparam_by_index(uint32_t index, power::EleParam_dkd_t &output) const { return power_db_->get_eleparam_from_index(index, output); }
    inline void show_database_ui(bool show = true) { power_db_->setVisible(show); }

signals:
    // 告知PowerPanel發生了改變
    void sig_ele_changed(const edm::power::EleParam_dkd_t& eleparam);

private:
    void _ele_cycle_timer_slot();

private:
    io::IOController::ptr io_ctrler_;
    power::PowerController::ptr power_ctrler_;

    PowerDatabase* power_db_;

private:
    QTimer* ele_cycle_timer_;
};

} // namespace app
} // namespace edm

