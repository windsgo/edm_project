#pragma once

#include <functional>
#include <memory>

#include <cstdint>

namespace edm {

namespace global {

class CommandBase {
public:
    using ptr = std::shared_ptr<CommandBase>;
    CommandBase() noexcept = default;
    virtual ~CommandBase() noexcept = default;

    virtual void run() = 0;
};

//! 推荐使用std::bind来绑定需要参数的命令 (可以是std::bind(lambda, args...))
//! 不推荐直接使用lambda表达式传入, 因为lambda表达式无法拷贝捕获的参数值
//! 采用队列方式运行时, 可能会出现参数值改变后, lambda表达式才被调用的情况
template <typename __CallableType, typename... __Args>
class CommandCommonFunction final : public CommandBase {
public:
    CommandCommonFunction(__CallableType &&callable, __Args &&...args)
        : CommandBase(),
          callable_{std::bind(std::forward<__CallableType>(callable),
                              std::forward<__Args>(args)...)} {}
    ~CommandCommonFunction() noexcept override = default;

    void run() override { callable_(); }

private:
    std::function<void(void)> callable_;
};

class CommandCommonFunctionFactory final {
public:
    CommandCommonFunctionFactory() = delete;

    template <typename __CallableType, typename... __Args>
    static inline auto bind(__CallableType &&callable, __Args &&...args) {
        return std::make_shared<
            CommandCommonFunction<__CallableType, __Args...>>(
            std::forward<__CallableType>(callable),
            std::forward<__Args>(args)...);
    }
};

} // namespace global

} // namespace edm
