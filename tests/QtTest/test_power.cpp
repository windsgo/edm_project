#include "edm.h"

#include <QCanBus>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include "QtDependComponents/CanController/CanController.h"
#include "QtDependComponents/IOController/IOController.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

#include <memory>

class Entity {
public:
    Entity(int x) : x_(x) {}

    int x() const { return x_; } 
    int& x() { return x_; }

private:
    int x_ = 0;
};

void add_to_entity(std::unique_ptr<Entity> e, int added) {
    e->x() += added;
}

std::unique_ptr<Entity> make_entity(int x) {
    auto e = std::make_unique<Entity>(x);
    return e;
}

int main(int argc, char **argv) {
    s_logger->debug("test power");

    QCoreApplication app(argc, argv);

    auto e = make_entity(123);
    add_to_entity(std::move(e), 222);

    s_logger->debug("e->x() = {}", e->x());

    return app.exec();
}