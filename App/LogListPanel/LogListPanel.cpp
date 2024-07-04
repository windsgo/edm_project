#include "LogListPanel.h"
#include "Logger/LogMacro.h"
#include "spdlog/common.h"
#include "spdlog/details/console_globals.h"
#include "spdlog/formatter.h"
#include "spdlog/logger.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"
#include "ui_LogListPanel.h"
#include <chrono>
#include <functional>
#include <memory>

#include <qcolor.h>
#include <qnamespace.h>
#include <qpushbutton.h>
#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <string>
#include <vector>

#include "Utils/Format/edm_format.h"

EDM_STATIC_LOGGER_NAME(s_logger, "loglist")

#include <QDebug>

namespace edm {
namespace app {
LogListPanel::LogListPanel(SharedCoreData *shared_core_data, QWidget *parent)
    : QWidget(parent), ui(new Ui::LogListPanel),
      shared_core_data_(shared_core_data) {
    ui->setupUi(this);

    auto callback_sink = std::make_shared<spdlog::sinks::callback_sink_st>(
        std::bind_front(&LogListPanel::_logged_callback, this));
    callback_sink->set_level(spdlog::level::trace);

    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_st>(
        "loglist.log", 10485760, 2, false);
    file_sink->set_level(spdlog::level::trace);

    std::vector<spdlog::sink_ptr> sinks{callback_sink, file_sink};

    loglist_logger_ =
        std::make_shared<spdlog::logger>("loglist", sinks.begin(), sinks.end());
    loglist_logger_->set_level(spdlog::level::trace);

    // spdlog::register_logger(loglist_logger_);

    // swap with s_logger, so that other place can use
    // `EDM_STATIC_LOGGER_NAME(s_logger, "loglist")`
    s_logger->swap(*loglist_logger_);
    loglist_logger_ = s_logger;

    formatter_.set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");

    shared_core_data_->set_loglist_logger(loglist_logger_);

    _init_pb();
}

void LogListPanel::_init_pb() {
    connect(ui->pb_clear, &QPushButton::clicked, this, [this]() {
        ui->listWidget->clear();
    });
}

void LogListPanel::_logged_callback(const spdlog::details::log_msg &msg) {
    // auto content_str = std::string{msg.payload.begin(), msg.payload.end()};

    // auto level_strv = spdlog::level::to_string_view(msg.level);
    // auto level_str = std::string{level_strv.begin(), level_strv.end()};

    // auto time = std::chrono::time_point_cast<std::chrono::seconds>(msg.time);
    // auto time_str = EDM_FMT::format("[{:%F %T}]", time);

    // qDebug().noquote() << QString::fromStdString(time_str)
    //                    << QString::fromStdString(level_str);

    spdlog::memory_buf_t dest;
    formatter_.format(msg, dest);
    auto str = std::string{dest.begin(), dest.end()};
    if (str.ends_with('\n')) {
        str.pop_back();
    }

    ui->listWidget->addItem(QString::fromStdString(str));
    auto item = ui->listWidget->item(ui->listWidget->count() - 1);
    if (item) {
        switch (msg.level) {
        case spdlog::level::level_enum::debug:
            item->setBackground(QColor{0, 191, 255});
            break;
        default:
        case spdlog::level::level_enum::trace:
            break;
        case spdlog::level::level_enum::info:
            item->setBackground(QColor{0, 238, 118});
            break;
        case spdlog::level::level_enum::warn:
            item->setBackground(QColor{255, 193, 37});
            break;
        case spdlog::level::level_enum::err:
        case spdlog::level::level_enum::critical:
            item->setBackground(QColor{255, 69, 0});
            item->setForeground(Qt::white);
            break;
        }
    }
}

LogListPanel::~LogListPanel() { delete ui; }

} // namespace app
} // namespace edm
