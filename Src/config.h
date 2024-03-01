#pragma once

// #define EDM_CONFIG_DIR "/home/windsgo/Documents/edmproject/edm_project/Conf/"
#define EDM_CONFIG_DIR                            EDM_ROOT_DIR "/Conf/"

#define EDM_LOG_CONFIG_FILE                       EDM_CONFIG_DIR "logdefine.json"

// 坐标轴(=驱动器)数目
#define EDM_SERVO_NUM                             3
#define EDM_AXIS_NUM                              EDM_SERVO_NUM

// 运动周期
#define EDM_SERVO_PEROID_US                       1000 // 1000 us 周期
#define EDM_SERVO_PEROID_NS                       (EDM_SERVO_PEROID_US * 1000) // 1000,000 ns 周期

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

// OFFLINE DEFINE 
// #define EDM_OFFLINE_RUN
#ifdef EDM_OFFLINE_RUN
#define EDM_OFFLINE_RUN_NO_ECAT
#define EDM_OFFLINE_RUN_NO_CAN
#endif // EDM_OFFLINE_RUN


#if __GNUC__ < 13      // gcc1 13.0之后支持 std::format
#define EDM_USE_FMTLIB // 使用fmtlib, 如果gcc不支持std::format
#endif
