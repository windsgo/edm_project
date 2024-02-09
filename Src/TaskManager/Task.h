#pragma once

#include <vector>
#include <memory>
#include <functional>

namespace edm
{

namespace task
{

class CommandBase {
public:
    using ptr = std::shared_ptr<CommandBase>;
    CommandBase() = default;
    virtual ~CommandBase() = default;

    virtual void run() = 0; // 执行command命令
};

} // namespace task

} // namespace edm


