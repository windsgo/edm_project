#include <QObject>
#include <QApplication>
#include <QCoreApplication>
#include <QWidget>

#include "QtDependComponents/PowerController/EleparamDefine.h"

#include <atomic>

#include "SharedCoreData/SharedCoreData.h"

#include "Logger/LogMacro.h"
EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

int main(int argc, char** argv) {

    QApplication app(argc, argv);

    using T = edm::power::CanIOBoardCommonMessageStrc;

    std::atomic<T> at_s;
    std::atomic<T> at_s2;

    s_logger->debug("lock free: {}", at_s.is_lock_free());

    T s;
    s.BZ_Once_flag = 1;
    at_s2.store(s);

    s_logger->debug("at_s.load().BZ_Once_flag: {}, {}", at_s.load().BZ_Once_flag, (int)s.BZ_Once_flag);

    s = at_s.exchange(at_s2);

    s_logger->debug("at_s.load().BZ_Once_flag: {}, {}", at_s.load().BZ_Once_flag, (int)s.BZ_Once_flag);

    // s = at_s.load();

    // at_s.store(s);

    auto scd = std::make_shared<edm::app::SharedCoreData>();

    auto v = scd->get_coord_system()->get_avaiable_coord_indexes();
    s_logger->debug("v: {}", v.size());

    return app.exec();

    return 0;
}