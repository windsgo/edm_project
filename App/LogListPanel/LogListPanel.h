#pragma once

#include <QWidget>
#include <string>

#include "Logger/LogDefine.h"
#include "Logger/LogMacro.h"
#include "spdlog/details/log_msg.h"
#include "spdlog/logger.h"
#include <spdlog/formatter.h>
#include <spdlog/pattern_formatter.h>

#include "SharedCoreData/SharedCoreData.h"

namespace Ui {
class LogListPanel;
}

namespace edm {
namespace app {


class LogListPanel : public QWidget {
    Q_OBJECT

public:
    explicit LogListPanel(SharedCoreData* shared_core_data, QWidget *parent = nullptr);
    ~LogListPanel();
    
    // get the logger instance to log outside
    inline auto get_logger_instance() const { return loglist_logger_; }

private:
    void _logged_callback(const spdlog::details::log_msg& msg);

    void _init_pb();

private:
    Ui::LogListPanel *ui;
    SharedCoreData* shared_core_data_;

    log::logger_ptr loglist_logger_;

    spdlog::pattern_formatter formatter_;
};

} // namespace app
} // namespace edm
