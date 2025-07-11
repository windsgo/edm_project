#pragma once

#define EDM_CONFIG_DIR                  EDM_ROOT_DIR "Conf/"
#define EDM_SYSTEM_SETTINGS_CONFIG_FILE EDM_CONFIG_DIR "system.json"


// 电柜使用定义
#define EDM_POWER_DIMEN                             1
#define EDM_POWER_ZHONGGU                           2
#define EDM_POWER_ZHONGGU_DRILL                     3   
#define EDM_POWER_TYPE                              EDM_POWER_DIMEN
// #define EDM_POWER_TYPE                              EDM_POWER_ZHONGGU_DRILL
#ifndef EDM_POWER_TYPE
#error "EDM_POWER_TYPE not valid"
#endif

#ifdef EDM_POWER_DIMEN
    #define EDM_POWER_DIMEN_WITH_EXTRA_ZHONGGU_IO // dimen电源, 但使用中谷的IO板控制部分IO(水泵,工件夹具,电极夹具)
#endif

// 使能Audio功能(做穿透检测实验时使用的)
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
#define EDM_ENABLE_AUDIO_RECORD
#endif

// 坐标轴(=驱动器)数目
#define EDM_AXIS_NUM                    6
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
#define EDM_SERVO_NUM                   EDM_AXIS_NUM + 1 // 多一个主轴
#define EDM_DRILL_S_AXIS_IDX            EDM_AXIS_NUM - 1 // S轴编号(0开始, 5)
#define EDM_DRILL_SPINDLE_AXIS_IDX      EDM_AXIS_NUM // 主轴旋转编号(0开始, 6)
#else 
#define EDM_SERVO_NUM                   EDM_AXIS_NUM
#endif // EDM_POWER_TYPE

#define EDM_AXIS_MAX_NUM                6 // const, 最多6轴
#if (EDM_AXIS_NUM > EDM_AXIS_MAX_NUM)
#error "EDM_AXIS_NUM > EDM_AXIS_MAX_NUM"
#endif

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
// 做一些覆盖性质定义
#define EDM_BLU_PER_UM 1
#endif // EDM_POWER_TYPE

// 单位定义, um分辨率
// #define EDM_BLU_PER_UM 10 // blu定义, 1blu为0.1um, 1um为10个blu
#ifndef EDM_BLU_PER_UM
#define EDM_BLU_PER_UM 1 // blu定义, 1blu为0.1um, 1um为10个blu
#endif // EDM_BLU_PER_UM

// 运动周期
// #define EDM_SERVO_PEROID_US                         1000 // 1000 us 周期
// #define EDM_SERVO_PEROID_NS                         (EDM_SERVO_PEROID_US *
// 1000) // 1000,000 ns 周期 #define EDM_SERVO_PEROID_MS
// ((double)EDM_SERVO_PEROID_US / 1000.0) #define EDM_SERVO_PEROID_MS_PER_PEROID
// (EDM_SERVO_PEROID_MS) #define EDM_SERVO_PEROID_PEROID_PER_MS (1.0 /
// EDM_SERVO_PEROID_MS)

// #define EDM_ECAT_DRIVER_SOEM                        // Use "soem" as software
// driver #define EDM_ECAT_DRIVER_IGH                    // or "igh", choose
// only one

#ifdef EDM_ECAT_DRIVER_IGH
#define EDM_ECAT_DRIVER_IGH_DC_SYNC_TO_MASTER 0
#define EDM_ECAT_DRIVER_IGH_DC_SYNC_TO_SLAVE0 1

#define EDM_ECAT_DRIVER_IGH_DC_MODE           EDM_ECAT_DRIVER_IGH_DC_SYNC_TO_SLAVE0
// #define EDM_ECAT_DRIVER_IGH_DC_MODE EDM_ECAT_DRIVER_IGH_DC_SYNC_TO_MASTER
#endif // EDM_ECAT_DRIVER_IGH

#if defined(EDM_ECAT_DRIVER_SOEM) && defined(EDM_ECAT_DRIVER_IGH)
#error "can not define both ethercat master"
#elif !defined(EDM_ECAT_DRIVER_SOEM) && !defined(EDM_ECAT_DRIVER_IGH)
#error "no ethercat master defined"
#endif

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
#define EDM_CUSTOM_QTEVENT_TYPE_MotionOPumpOn       1012
#define EDM_CUSTOM_QTEVENT_TYPE_MotionIPumpOn       1013
#define EDM_CUSTOM_QTEVENT_TYPE_MotionTriggerBzOnce 1014

// CAN TxID 集合 (DIMEN)
#define EDM_CAN_TXID_POWER                          0x0010
#define EDM_CAN_TXID_CANIO_1                        0x0456
#define EDM_CAN_TXID_CANIO_2                        0x0458
#define EDM_CAN_TXID_IOBOARD_SERVOSETTING           0x0401
#define EDM_CAN_TXID_IOBOARD_COMMONMESSAGE          0x0402
#define EDM_CAN_TXID_IOBOARD_ELEPARAMS              0x0403
#define EDM_CAN_RXID_IOBOARD_SERVODATA              0x0231
#define EDM_CAN_RXID_IOBOARD_ADCINFO                0x0232
#define EDM_CAN_TXID_HANDBOX_IOSTATUS               0x0501
#define EDM_CAN_RXID_IOBOARD_DIRECTOR_STATE         0x0703
#define EDM_CAN_TXID_IOBOARD_DIRECTOR_STARTPOINTMOVE        0x0603
#define EDM_CAN_TXID_IOBOARD_DIRECTOR_STOPPOINTMOVE         0x0604
#define EDM_CAN_TXID_IOBOARD_DIRECTOR_STARTHOMEMOVE         0x0605

// CAN ID (ZHONGGU)
#define EDM_CAN_ZHONGGU_IO_OUTPUT_TXID              0x601 // 407,103统一的IO发送ID
#define EDM_CAN_ZHONGGU_IO_INPUT_RXID              0x701 // 103 返回INPUT IO

#define EDM_USE_ZYNQ_SERVOBOARD

// DataQueueRecorder Cache
#define EDM_DATAQUEUERECORDER_ENABLE_CACHE // 使能cache缓存

// Motion Thread Stack Size
#define EDM_MOTION_THREAD_STACK            (64 * 1024)

// MotionSignalQueue 使用的queue类型
#define EDM_MOTION_SIGNAL_QUEUE_USE_SPSC   // 使用spsc队列

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
    4 // 本地调试CAN, 不连接ECAT, 启动实时线程, 要求2个CAN设备互联,
      // 模拟测试压力, 但是不取CAN返回值
#define EDM_OFFLINE_RUN_TYPE_5 \
    5 // 调试CAN, 不连接ECAT, 启动实时线程, 但使用CAN返回的接触感知(连接IO板)
#define EDM_OFFLINE_RUN_TYPE_6 \
    6 // 连接ECAT, 2个CAN互连, 模拟测试压力, 但是不取CAN返回值

// 20240813后新增 // TODO
#define EDM_OFFLINE_RUN_TYPE_7 \
    7 // 不连接ECAT, 不连接CAN, 不连接ZYNQ
#define EDM_OFFLINE_RUN_TYPE_8 \
    8 // 不连接ECAT, 不连接CAN, 连接ZYNQ, 采用ZYNQ返回的伺服信息
#define EDM_OFFLINE_RUN_TYPE_9 \
    9 // 不连接ECAT, 连接CAN(操作中谷IO), 连接ZYNQ(伺服信息)

#define EDM_OFFLINE_RUN_TYPE EDM_OFFLINE_RUN_TYPE_2 //! Choose an OFFLINE type

#ifdef EDM_OFFLINE_RUN //! OFFLINE DEFINE START

#define EDM_OFFLINE_RUN_NO_ECAT         // 离线不连接ecat, 不发送ecat指令
#define EDM_OFFLINE_RUN_NO_CAN          // 离线不连接CAN设备
#define EDM_OFFLINE_RUN_NO_ZYNQ         // 离线不连接ZYNQ设备
#define EDM_OFFLINE_MANUAL_TOUCH_DETECT // 离线手动按钮标志接触感知
#define EDM_OFFLINE_MANUAL_SERVO_CMD    // 离线手动按钮/滑动条标志伺服速度
#define EDM_OFFLINE_MANUAL_VOLTAGE      // 离线手动电压(调试电压实时显示)

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
#undef EDM_OFFLINE_RUN_NO_CAN
#undef EDM_OFFLINE_MANUAL_TOUCH_DETECT // 取消此定义, 采用CAN返回的接触感知信号
#elif (EDM_OFFLINE_RUN_TYPE == EDM_OFFLINE_RUN_TYPE_6)
#undef EDM_OFFLINE_RUN_NO_ECAT // 取消此定义, 并连接ECAT
#undef EDM_OFFLINE_RUN_NO_CAN
#define EDM_OFFLINE_RUN_MANUAL_TWO_CAN_DEVICE // 离线测试时, 连接两个CAN设备,
                                              // 用于保持正常的can通信链路

#elif (EDM_OFFLINE_RUN_TYPE == EDM_OFFLINE_RUN_TYPE_7)
#elif (EDM_OFFLINE_RUN_TYPE == EDM_OFFLINE_RUN_TYPE_8)
    #undef EDM_OFFLINE_RUN_NO_ZYNQ
    #undef EDM_OFFLINE_MANUAL_TOUCH_DETECT
    #undef EDM_OFFLINE_MANUAL_SERVO_CMD
    #undef EDM_OFFLINE_MANUAL_VOLTAGE
#elif (EDM_OFFLINE_RUN_TYPE == EDM_OFFLINE_RUN_TYPE_9)
    #undef EDM_OFFLINE_RUN_NO_CAN
    #undef EDM_OFFLINE_RUN_NO_ZYNQ
    #undef EDM_OFFLINE_MANUAL_TOUCH_DETECT
    #undef EDM_OFFLINE_MANUAL_SERVO_CMD
    #undef EDM_OFFLINE_MANUAL_VOLTAGE
#else
#error "No EDM_OFFLINE_RUN_TYPE defined"
#endif // EDM_OFFLINE_RUN_TYPE value

#endif // EDM_OFFLINE_RUN //! OFFLINE DEFINE END

#if __GNUC__ < 13      // gcc1 13.0之后支持 std::format
#define EDM_USE_FMTLIB // 使用fmtlib, 如果gcc不支持std::format
#endif
