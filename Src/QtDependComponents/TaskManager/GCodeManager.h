#pragma once

#include <QObject>
#include <QTimer>

#include <memory>

namespace edm
{

namespace task
{

class GCodeManager final : public QObject {
    Q_OBJECT
public:
    using ptr = std::shared_ptr<GCodeManager>;
    GCodeManager();
    ~GCodeManager();

public slots: // 接受info提供的信号, 告知G代码运行状态切换
    // TODO
};

} // namespace task

} // namespace edm

