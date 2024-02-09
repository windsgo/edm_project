#pragma once

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

#include "json.hpp" // meojson

namespace edm {
namespace interpreter {

// 统一的异常类, 简便起见, 主要是throw解释器的逻辑、传值类型错误,
// 以及Python解释器所设置的错误, 如语法错误, 运行时错误等
class RS274InterpreterException : public ::std::exception {
public:
    RS274InterpreterException(::std::string_view type, ::std::string_view desc)
        : err_type_(type), err_desc_(desc) {
        ::std::stringstream ss;
        ss << "** RS274InterpreterException: \n** Type: " << err_type_
           << "\n** Description: " << err_desc_;
        what_msg_ = std::move(ss.str());
    }

    virtual const char *what() const noexcept override final {
        return what_msg_.c_str();
    }

    const char *type() const noexcept { return err_type_.c_str(); }

    const char *desc() const noexcept { return err_desc_.c_str(); }

private:
    ::std::string err_type_;
    ::std::string err_desc_;
    ::std::string what_msg_;
};

// class RS274InterpreterWrapperImpl;

// use python3 interpreter
// need GIL, thus each process can launch only 1 python3 interpreter at one time
// so, use Singleton Mode here

// 类不可重入, 线程不安全

// Singleton Class
class RS274InterpreterWrapper final {
public:
    using json_value = ::json::value;

public:
    // static method: get instance
    static RS274InterpreterWrapper *instance();

    // static method: parse a file, if an error happens, an exception will be
    // thrown 字符串返回
    static ::std::string parse_file(std::string_view filename);

    // static method: parse a file, if an error happens, an exception will be
    // thrown json返回
    static json_value parse_file_to_json(std::string_view filename);

public:
    // member functions

    // 设置rs274.py所在路径, 建议给绝对路径,
    // 可以通过 __FILE__ 宏定义获取源代码的路径, 并通过附加相对路径格式得到
    // rs274.py 的路径 用于在解释器启动时, 加载 rs274.py 模块
    void set_rs274_py_module_dir(::std::string_view module_dir) {
        rs274_module_dir_ = module_dir;
    }
    ::std::string_view get_rs274_py_module_dir() const {
        return rs274_module_dir_;
    }

private:
    ::std::string rs274_module_dir_;
    // std::shared_ptr<RS274InterpreterWrapperImpl> impl_;

private:
    static RS274InterpreterWrapper s_instance;

private:
    // default ctor
    RS274InterpreterWrapper() noexcept = default;

    // dtor
    ~RS274InterpreterWrapper() noexcept = default;

    // disable copy & move
    RS274InterpreterWrapper(const RS274InterpreterWrapper &) = delete;
    RS274InterpreterWrapper(RS274InterpreterWrapper &&) = delete;
};

} // namespace interpreter
} // namespace edm
