#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QObject>
#include <QWidget>

#include "MainWindow/MainWindow.h"

#include "Logger/LogMacro.h"
EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

static void init_sys_qss(void) {
    QFile stylefile(QString::fromStdString(
        EDM_CONFIG_DIR + edm::SystemSettings::instance().get_qss_file()));
    stylefile.open(QFile::ReadOnly);
    if (stylefile.isOpen()) {
        qApp->setStyleSheet(stylefile.readAll());
    } else {
        s_logger->error("style sheet error");
    }
    stylefile.close();
}

static void print_sys_start() {
    s_logger->info("-----------------------------------------");
    s_logger->info("------------ System Started -------------");
    s_logger->info("-----------------------------------------");
}

using namespace edm::app;

int main(int argc, char **argv) {

    print_sys_start();

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication a(argc, argv);

    init_sys_qss();

    MainWindow m;
    m.show();

    return a.exec();

    return 0;
}
