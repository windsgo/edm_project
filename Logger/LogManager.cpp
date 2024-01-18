#include <LogManager.h>

namespace edm {

namespace log {

LogManager *LogManager::instance() {
    // 使用懒汉式, 第一次调用时初始化,
    // 可以保证初始化顺序在调用LogManager方法之前
    static LogManager instance;
    return &instance;
}

logger_ptr LogManager::get_logger(const std::string & logger_name) {
    return logger_ptr();
}

logger_ptr LogManager::get_root_logger() const { 
    return root_logger_; 
}

} // namespace log

} // namespace edm
