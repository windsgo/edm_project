#pragma once

#define EDM_CONFIG_DIR                  EDM_ROOT_DIR "Conf/"
#define EDM_SYSTEM_SETTINGS_CONFIG_FILE EDM_CONFIG_DIR "system.json"

// 坐标轴(=驱动器)数目
#define EDM_SERVO_NUM                   3
#define EDM_AXIS_NUM                    EDM_SERVO_NUM

#define EDM_AXIS_MAX_NUM                6 // const, 最多6轴
#if (EDM_AXIS_NUM > EDM_AXIS_MAX_NUM)
#error "EDM_AXIS_NUM > EDM_AXIS_MAX_NUM"
#endif

// 单位定义, um分辨率
#define EDM_BLU_PER_UM                            10 // blu定义, 1blu为0.1um, 1um为10个blu

// 运动周期
#define EDM_SERVO_PEROID_US                       2000 // 1000 us 周期
#define EDM_SERVO_PEROID_NS                       (EDM_SERVO_PEROID_US * 1000) // 1000,000 ns 周期
#define EDM_SERVO_PEROID_MS                       ((double)EDM_SERVO_PEROID_US / 1000.0)
#define EDM_SERVO_PEROID_MS_PER_PEROID            (EDM_SERVO_PEROID_MS)
#define EDM_SERVO_PEROID_PEROID_PER_MS            (1.0 / EDM_SERVO_PEROID_MS)

#define EDM_ECAT_DRIVER_SOEM                      // Use "soem" as software driver
// #define EDM_ECAT_DRIVER_IGH                    // or "igh", choose only one

#define EDM_CAN_SET_DOWN_WHEN_WORKER_DELETED      // canworker析构时,
                                                  // 将can设备设置down

// 自定义Qt事件号
#define EDM_CUSTOM_QTEVENT_TYPE_CanSendFrameEvent 1001
#define EDM_CUSTOM_QTEVENT_TYPE_CanStartEvent     1002

#define EDM_CUSTOM_QTEVENT_TYPE_HandboxStartPM    1003
#define EDM_CUSTOM_QTEVENT_TYPE_HandboxStopPM     1004
#define EDM_CUSTOM_QTEVENT_TYPE_HandboxPump       1005
#define EDM_CUSTOM_QTEVENT_TYPE_HandboxEntAuto    1006
#define EDM_CUSTOM_QTEVENT_TYPE_HandboxPauseAuto  1007
#define EDM_CUSTOM_QTEVENT_TYPE_HandboxStopAuto   1008
#define EDM_CUSTOM_QTEVENT_TYPE_HandboxAck        1009

// CAN TxID 集合
#define EDM_CAN_TXID_POWER                        0x0010
#define EDM_CAN_TXID_CANIO_1                      0x0356
#define EDM_CAN_TXID_CANIO_2                      0x0358
#define EDM_CAN_TXID_IOBOARD_SERVOSETTING         0x0401
#define EDM_CAN_TXID_IOBOARD_COMMONMESSAGE        0x0402
#define EDM_CAN_TXID_IOBOARD_ELEPARAMS            0x0403
#define EDM_CAN_RXID_IOBOARD_SERVODATA            0x0231
#define EDM_CAN_RXID_IOBOARD_ADCINFO              0x0232

// DataQueueRecorder Cache
#define EDM_DATAQUEUERECORDER_ENABLE_CACHE        // 使能cache缓存

// Motion Thread Stack Size
#define EDM_MOTION_THREAD_STACK                   (64 * 1024)

// MotionSignalQueue 使用的queue类型
#define EDM_MOTION_SIGNAL_QUEUE_USE_SPSC          // 使用spsc队列

// 不合理的coord config 抛出异常, 而不是自动使用默认
#define EDM_INVALID_COORD_CONFIG_THROW

// OFFLINE DEFINE
#define EDM_OFFLINE_RUN

#ifdef EDM_OFFLINE_RUN
#ifndef EDM_OFFLINE_RUN_NO_ECAT
#define EDM_OFFLINE_RUN_NO_ECAT // 离线不连接ecat, 不发送ecat指令
#endif // EDM_OFFLINE_RUN_NO_ECAT
// #ifndef EDM_OFFLINE_RUN_NO_CAN
// #define EDM_OFFLINE_RUN_NO_CAN // TODO
// #endif                         // EDM_OFFLINE_RUN_NO_CAN

// 离线测试时, 连接两个CAN设备, 用于保持正常的can通信链路
#define EDM_OFFLINE_RUN_MANUAL_TWO_CAN_DEVICE

#ifndef EDM_OFFLINE_MANUAL_TOUCH_DETECT
#define EDM_OFFLINE_MANUAL_TOUCH_DETECT // 离线手动按钮标志接触感知
#endif // EDM_OFFLINE_MANUAL_TOUCH_DETECT
#ifndef EDM_OFFLINE_MANUAL_SERVO_CMD
#define EDM_OFFLINE_MANUAL_SERVO_CMD // 离线手动按钮/滑动条标志伺服速度
#endif // EDM_OFFLINE_MANUAL_SERVO_CMD
#endif // EDM_OFFLINE_RUN

#if __GNUC__ < 13      // gcc1 13.0之后支持 std::format
#define EDM_USE_FMTLIB // 使用fmtlib, 如果gcc不支持std::format
#endif
