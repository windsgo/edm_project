#pragma once

#include "Logger/LogManager.h"

#define EDM_GET_LOG_MANAGER() edm::log::LogManager::instance()

#define EDM_LOGGER(logger_name_) \
    EDM_GET_LOG_MANAGER()->get_logger((logger_name_))

#define EDM_LOGGER_ROOT() EDM_GET_LOG_MANAGER()->get_root_logger() // faster get

#define EDM_STATIC_LOGGER_NAME(var_name_, logger_name_) \
    static auto var_name_ = EDM_LOGGER((logger_name_));
#define EDM_STATIC_LOGGER(var_name_, logger_) static auto var_name_ = (logger_);

/* 调试打印宏 */
#define EDM_CYCLIC_LOG(__func, __peroid, fmt, ...) \
    {                                              \
        static int __cyclic_i = 0;                 \
        ++__cyclic_i;                              \
        if (__cyclic_i >= (__peroid)) {            \
            __func(fmt, ##__VA_ARGS__);            \
            __cyclic_i = 0;                        \
        }                                          \
    }

