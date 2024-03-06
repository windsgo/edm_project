#pragma once

// #define EDM_CONFIG_DIR "/home/windsgo/Documents/edmproject/edm_project/Conf/"
#define EDM_CONFIG_DIR                            EDM_ROOT_DIR "/Conf/"

#define EDM_LOG_CONFIG_FILE                       EDM_CONFIG_DIR "logdefine.json"

// 坐标轴(=驱动器)数目
#define EDM_SERVO_NUM                             1
#define EDM_AXIS_NUM                              EDM_SERVO_NUM

// 单位定义
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

// CAN TxID 集合
#define EDM_CAN_TXID_POWER                        0x0010
#define EDM_CAN_TXID_CANIO_1                      0x0356
#define EDM_CAN_TXID_CANIO_2                      0x0358
#define EDM_CAN_TXID_IOBOARD_SERVOSETTING         0x0401
#define EDM_CAN_TXID_IOBOARD_COMMONMESSAGE        0x0402
#define EDM_CAN_TXID_IOBOARD_ELEPARAMS            0x0403

// DataQueueRecorder Cache
#define EDM_DATAQUEUERECORDER_ENABLE_CACHE        // 使能cache缓存

// Motion Thread Stack Size
#define EDM_MOTION_THREAD_STACK                   (64 * 1024)

// MotionSignalQueue 使用的queue类型
#define EDM_MOTION_SIGNAL_QUEUE_USE_SPSC          // 使用spsc队列

// OFFLINE DEFINE
// #define EDM_OFFLINE_RUN
#ifdef EDM_OFFLINE_RUN
#ifndef EDM_OFFLINE_RUN_NO_ECAT
#define EDM_OFFLINE_RUN_NO_ECAT
#endif // EDM_OFFLINE_RUN_NO_ECAT
#ifndef EDM_OFFLINE_RUN_NO_CAN
#define EDM_OFFLINE_RUN_NO_CAN // TODO
#endif                         // EDM_OFFLINE_RUN_NO_CAN
// TODO Touch Detect
// TODO ServoDir ServoDis
#endif // EDM_OFFLINE_RUN

#if __GNUC__ < 13      // gcc1 13.0之后支持 std::format
#define EDM_USE_FMTLIB // 使用fmtlib, 如果gcc不支持std::format
#endif
