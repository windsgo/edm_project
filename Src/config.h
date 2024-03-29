#pragma once

#define EDM_CONFIG_DIR                  EDM_ROOT_DIR "Conf/"
#define EDM_SYSTEM_SETTINGS_CONFIG_FILE EDM_CONFIG_DIR "system.json"

// 坐标轴(=驱动器)数目
#define EDM_SERVO_NUM                   1
#define EDM_AXIS_NUM                    EDM_SERVO_NUM

#define EDM_AXIS_MAX_NUM                6 // const, 最多6轴
#if (EDM_AXIS_NUM > EDM_AXIS_MAX_NUM)
#error "EDM_AXIS_NUM > EDM_AXIS_MAX_NUM"
#endif

// 单位定义, um分辨率
#define EDM_BLU_PER_UM                              10 // blu定义, 1blu为0.1um, 1um为10个blu

// 运动周期
#define EDM_SERVO_PEROID_US                         1000 // 1000 us 周期
#define EDM_SERVO_PEROID_NS                         (EDM_SERVO_PEROID_US * 1000) // 1000,000 ns 周期
#define EDM_SERVO_PEROID_MS                         ((double)EDM_SERVO_PEROID_US / 1000.0)
#define EDM_SERVO_PEROID_MS_PER_PEROID              (EDM_SERVO_PEROID_MS)
#define EDM_SERVO_PEROID_PEROID_PER_MS              (1.0 / EDM_SERVO_PEROID_MS)

#define EDM_ECAT_DRIVER_SOEM                        // Use "soem" as software driver
// #define EDM_ECAT_DRIVER_IGH                    // or "igh", choose only one

#define EDM_CAN_SET_DOWN_WHEN_WORKER_DELETED        // canworker析构时,
                                                    // 将can设备设置down

// 自定义Qt事件号
#define EDM_CUSTOM_QTEVENT_TYPE_CanSendFrameEvent   1001
#define EDM_CUSTOM_QTEVENT_TYPE_CanStartEvent       1002

#define EDM_CUSTOM_QTEVENT_TYPE_HandboxStartPM      1003
#define EDM_CUSTOM_QTEVENT_TYPE_HandboxStopPM       1004
#define EDM_CUSTOM_QTEVENT_TYPE_HandboxPump         1005
#define EDM_CUSTOM_QTEVENT_TYPE_HandboxEntAuto      1006
#define EDM_CUSTOM_QTEVENT_TYPE_HandboxPauseAuto    1007
#define EDM_CUSTOM_QTEVENT_TYPE_HandboxStopAuto     1008
#define EDM_CUSTOM_QTEVENT_TYPE_HandboxAck          1009

#define EDM_CUSTOM_QTEVENT_TYPE_MotionVoltageEnable 1010
#define EDM_CUSTOM_QTEVENT_TYPE_MotionMachOn        1011

// CAN TxID 集合
#define EDM_CAN_TXID_POWER                          0x0010
#define EDM_CAN_TXID_CANIO_1                        0x0356
#define EDM_CAN_TXID_CANIO_2                        0x0358
#define EDM_CAN_TXID_IOBOARD_SERVOSETTING           0x0401
#define EDM_CAN_TXID_IOBOARD_COMMONMESSAGE          0x0402
#define EDM_CAN_TXID_IOBOARD_ELEPARAMS              0x0403
#define EDM_CAN_RXID_IOBOARD_SERVODATA              0x0231
#define EDM_CAN_RXID_IOBOARD_ADCINFO                0x0232

// DataQueueRecorder Cache
#define EDM_DATAQUEUERECORDER_ENABLE_CACHE          // 使能cache缓存

// Motion Thread Stack Size
#define EDM_MOTION_THREAD_STACK                     (64 * 1024)

// MotionSignalQueue 使用的queue类型
#define EDM_MOTION_SIGNAL_QUEUE_USE_SPSC            // 使用spsc队列

// 不合理的coord config 抛出异常, 而不是自动使用默认
// #define EDM_INVALID_COORD_CONFIG_THROW

// 使能时间统计
#define EDM_ENABLE_TIMEUSE_STAT

// 使用原子操作获取info
#define EDM_MOTION_INFO_GET_USE_ATOMIC

// 使用新的IO板伺服返回协议(1ms返回一次, 并全部整合在一起)
#define EDM_IOBOARD_NEW_SERVODATA_1MS

// OFFLINE DEFINE
#define EDM_OFFLINE_RUN

#define EDM_OFFLINE_RUN_TYPE_1 1 // 完全不连接任何设备, 也不启动实时线程
#define EDM_OFFLINE_RUN_TYPE_2 2 // 完全不连接任何设备, 但是启动实时线程
#define EDM_OFFLINE_RUN_TYPE_3 3 // 调试ECAT, 不连接CAN, 启动实时线程
#define EDM_OFFLINE_RUN_TYPE_4 \
    4 // 本地调试CAN, 不连接ECAT, 启动实时线程, 要求2个CAN设备互联, 模拟测试压力, 但是不取CAN返回值
#define EDM_OFFLINE_RUN_TYPE_5 \
    5 // 调试CAN, 不连接ECAT, 启动实时线程, 但使用CAN返回的接触感知(连接IO板)
#define EDM_OFFLINE_RUN_TYPE_6 6// 连接ECAT, 2个CAN互连, 模拟测试压力, 但是不取CAN返回值

#define EDM_OFFLINE_RUN_TYPE EDM_OFFLINE_RUN_TYPE_3 //! Choose an OFFLINE type

#ifdef EDM_OFFLINE_RUN //! OFFLINE DEFINE START

#define EDM_OFFLINE_RUN_NO_ECAT         // 离线不连接ecat, 不发送ecat指令
#define EDM_OFFLINE_RUN_NO_CAN          // 离线不连接CAN设备
#define EDM_OFFLINE_MANUAL_TOUCH_DETECT // 离线手动按钮标志接触感知
#define EDM_OFFLINE_MANUAL_SERVO_CMD    // 离线手动按钮/滑动条标志伺服速度

#if (EDM_OFFLINE_RUN_TYPE == EDM_OFFLINE_RUN_TYPE_1)
#define EDM_OFFLINE_NO_REALTIME_THREAD // 离线不启动实时线程
#elif (EDM_OFFLINE_RUN_TYPE == EDM_OFFLINE_RUN_TYPE_2)
// Do Nothing
#elif (EDM_OFFLINE_RUN_TYPE == EDM_OFFLINE_RUN_TYPE_3)
#undef EDM_OFFLINE_RUN_NO_ECAT // 取消此定义, 并连接ECAT
#elif (EDM_OFFLINE_RUN_TYPE == EDM_OFFLINE_RUN_TYPE_4)
#undef EDM_OFFLINE_RUN_NO_CAN
#define EDM_OFFLINE_RUN_MANUAL_TWO_CAN_DEVICE // 离线测试时, 连接两个CAN设备,
                                              // 用于保持正常的can通信链路
#elif (EDM_OFFLINE_RUN_TYPE == EDM_OFFLINE_RUN_TYPE_5)
#undef EDM_OFFLINE_MANUAL_TOUCH_DETECT // 取消此定义, 采用CAN返回的接触感知信号
#elif (EDM_OFFLINE_RUN_TYPE == EDM_OFFLINE_RUN_TYPE_6)
#undef EDM_OFFLINE_RUN_NO_ECAT // 取消此定义, 并连接ECAT
#undef EDM_OFFLINE_RUN_NO_CAN
#define EDM_OFFLINE_RUN_MANUAL_TWO_CAN_DEVICE // 离线测试时, 连接两个CAN设备,
                                              // 用于保持正常的can通信链路
#else
#error "No EDM_OFFLINE_RUN_TYPE defined"
#endif // EDM_OFFLINE_RUN_TYPE value

#endif // EDM_OFFLINE_RUN //! OFFLINE DEFINE END

#if __GNUC__ < 13      // gcc1 13.0之后支持 std::format
#define EDM_USE_FMTLIB // 使用fmtlib, 如果gcc不支持std::format
#endif
