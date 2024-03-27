#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QObject>
#include <QWidget>

#include <sched.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

#include "CanReceiveBuffer/CanReceiveBuffer.h"

#include "MainWindow/MainWindow.h"

#include "Logger/LogMacro.h"
EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

static void init_sys_qss(void) {
    QFile stylefile(QString::fromStdString(
        EDM_CONFIG_DIR + edm::SystemSettings::instance().get_qss_file()));
    // QFile stylefile(":/app/ui/qss/appgui.qss");
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
    s_logger->info("sizeof(Can1IOBoard407ServoData): {}", sizeof(edm::Can1IOBoard407ServoData));
}

static void init_mainthread_cpu_affinity() {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(4, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask)) {
        s_logger->error("set main thread affinity failed: pid: {}", (unsigned int)pthread_self());
    }
}

using namespace edm::app;

int main(int argc, char **argv) {

    // init_mainthread_cpu_affinity();

    print_sys_start();

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication a(argc, argv);

    init_sys_qss();

    MainWindow m;
    m.show();

    return a.exec();

    return 0;
}
