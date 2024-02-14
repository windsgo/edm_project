#pragma once

// #define EDM_CONFIG_DIR "/home/windsgo/Documents/edmproject/edm_project/Conf/"
#define EDM_CONFIG_DIR EDM_ROOT_DIR "/Conf/"

#define EDM_LOG_CONFIG_FILE EDM_CONFIG_DIR "logdefine.json"

// 坐标轴(=驱动器)数目
#define EDM_SERVO_NUM 3
#define EDM_AXIS_NUM EDM_SERVO_NUM

#define EDM_ECAT_DRIVER_SOEM // Use "soem" as software driver
#define EDM_ECAT_DRIVER_IGH // or "igh", choose only one

#define EDM_CAN_SET_DOWN_WHEN_WORKER_DELETED // canworker析构时, 将can设备设置down

#if __GNUC__ < 13 // gcc1 13.0之后支持 std::format
#define EDM_USE_FMTLIB // 使用fmtlib, 如果gcc不支持std::format
#endif
