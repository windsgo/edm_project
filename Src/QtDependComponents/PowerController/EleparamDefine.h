#pragma once

#include <cstdint>
#include <memory>

#include "config.h"

namespace edm {

namespace power {

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
#define DEC_LV0_CONTACTORS      0
#define DEC_LV1_CONTACTORS      2
#define DEC_LV2_CONTACTORS      3

/* 继电器IO定义 */
//---------------电容继电器----------------
#define DEC_CAP_NO_CONTACTORS   0
#define DEC_CAP1_CONTACTORS     22
#define DEC_CAP2_CONTACTORS     23
#define DEC_CAP3_CONTACTORS     24
#define DEC_CAP4_CONTACTORS     25
#define DEC_CAP5_CONTACTORS     26
#define DEC_CAP6_CONTACTORS     27
#define DEC_CAP7_CONTACTORS     28
#define DEC_CAP8_CONTACTORS     29
#define DEC_CAP9_CONTACTORS     30

//----------------低压遮断继电器-------------
#define DEC_INTERD1_CONTACTORS  0 // #F5
#define DEC_INTERD2_CONTACTORS  1 // #F9
#define DEC_INTERD3_CONTACTORS  2 // #FA
#define DEC_INTERD4_CONTACTORS  3 // #FE

//---------------极性继电器------------------
#define DEC_NEGATIVE_CONTACTORS 0
#define DEC_POSITIVE_CONTACTORS 39

//-----------------其他电参数继电器在CAN发送帧中的字节位置------------
#define DEC_IP0_CONTACTORS      42
#define DEC_HON_CONTACTORS      34
#define DEC_MON_CONTACTORS      35
#define DEC_NOW_CONTACTORS      44
#define DEC_MACH_CONTACTORS     31
#define DEC_FULD_CONTACTORS     38
#define DEC_BZ_CONTACTORS       33
#define DEC_ALLST_CONTACTORS    7
#define DEC_PK_CONTACTORS       37
#define DEC_IP7_CONTACTORS      43
#define DEC_IP15_CONTACTORS     46

enum EleContactorsOut {
    EleContactorOut_Unknown = 0, // OUT 1 ~ OUT 32, OUT33 ~ OUT 64 (从1开始编码)

    //! IO1部分
    EleContactorOut_V1_JV1 = 2, // V1, OUT2
    EleContactorOut_V2_JV2 = 3, // V2, OUT3

    // 电容继电器, 选择哪个电容就吸合哪个, 不能多选(每次设置吸合前要全部清空)
    EleContactorOut_C0_JC0 = 21, // C0, OUT21
    EleContactorOut_C1_JC1 = 22, // C1, OUT22
    EleContactorOut_C2_JC2 = 23, // C2, OUT23
    EleContactorOut_C3_JC3 = 24, // C3, OUT24
    EleContactorOut_C4_JC4 = 25, // C4, OUT25
    EleContactorOut_C5_JC5 = 26, // C5, OUT26
    EleContactorOut_C6_JC6 = 27, // C6, OUT27
    EleContactorOut_C7_JC7 = 28, // C7, OUT28
    EleContactorOut_C8_JC8 = 29, // C8, OUT29
    EleContactorOut_C9_JC9 = 30, // C9, OUT30

    //
    EleContactorOut_MACH_JF0 = 31, // MACH, OUT31, 脉冲电源

    //! PowerOn, OUT32, 强电启动 (不由电参数控制, 外部单独io)
    EleContactorOut_PWON_JF1 = 32, 

    //! IO2部分
    EleContactorOut_BZ_JF2 = 33, //! BZ, OUT33, 蜂鸣器(不由电参数控制)
    EleContactorOut_HON_JF3 = 34, // HON, OUT34, 高压遮断
    EleContactorOut_MON_JF4 = 35, // MON, OUT35, 中压遮断
    EleContactorOut_UNUSED_JF5 = 36, //// JF5 OUT36 不用
    EleContactorOut_PK_JF6 = 37, //! PK, OUT37, 硬件反逻辑( 置0为吸合, 1为断开 )
    EleContactorOut_FULD_JF7 = 38, //! FULD, OUT38, 油泵(不由电参数控制)
    EleContactorOut_RVNM_JF8 = 39, // RV/NM, OUT39, 极性控制(pl=1正极性时吸合)
    EleContactorOut_PK0_JF9 = 40, // PK0, OUT40
    EleContactorOut_UNUSED_JFA = 41,//// JFA OUT41 不用
    EleContactorOut_IP0_JFB = 42, //! IP0, OUT42 硬件反逻辑( 置0为吸合, 1为断开 )
    EleContactorOut_IP7_JFC = 43, // IP7, OUT43
    EleContactorOut_NOW_JFD = 44, //! NOW, OUT44, 硬件反逻辑( 置0为吸合, 1为断开 )
    EleContactorOut_UNUSED_JFE = 45,//// JCE OUT45 不用
    EleContactorOut_IP15_JFF = 46, // IP15, OUT46
    
    //! SOF 目前在OUT47, 与PowerOn一起启动(不由电参数控制, 外部单独io)
    EleContactorOut_SOF = 47, 

    EleContactorOut_MAX, // 定义当前最大枚举值
};

/* 继电器开关定义 */
#define CONTACTOR_DISABLE 0 // 断开继电器
#define CONTACTOR_ENABLE  1 // 吸合继电器

#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU)
enum ZHONGGU_IOOut {
    // 407 IO
    ZHONGGU_IOOut_IOOUT1_FULD = 1,
    ZHONGGU_IOOut_IOOUT2_NEG = 2,
    ZHONGGU_IOOut_IOOUT3_MACH = 3,
    ZHONGGU_IOOut_IOOUT4_BZ = 4,

    // 103 IO
    ZHONGGU_IOOut_IOOUT5_LIGHT = 5,
    ZHONGGU_IOOut_IOOUT6_RED = 6,
    ZHONGGU_IOOut_IOOUT7_YELLOW = 7,
    ZHONGGU_IOOut_IOOUT8_GREEN = 8,
    ZHONGGU_IOOut_IOOUT9_TOOL_FIXTURE = 9,
    ZHONGGU_IOOut_IOOUT10_WORK_FIXTURE = 10,
    ZHONGGU_IOOut_IOOUT11_HP1 = 11,
    ZHONGGU_IOOut_IOOUT12_HP2 = 12,

    ZHONGGU_IOOut_TON1 = 13,
    ZHONGGU_IOOut_TON2 = 14,
    ZHONGGU_IOOut_TON4 = 15,
    ZHONGGU_IOOut_TON8 = 16,
    ZHONGGU_IOOut_TON16 = 17,

    ZHONGGU_IOOut_TOFF1 = 18,
    ZHONGGU_IOOut_TOFF2 = 19,
    ZHONGGU_IOOut_TOFF4 = 20,
    ZHONGGU_IOOut_TOFF8 = 21,

    ZHONGGU_IOOut_IP1 = 22,
    ZHONGGU_IOOut_IP2 = 23,
    ZHONGGU_IOOut_IP4 = 24,
    ZHONGGU_IOOut_IP8 = 25,
};

enum ZHONGGU_IOIn {
    ZHONGGU_IOIn_IOIN1_PRESSURE = 1,
    ZHONGGU_IOIn_IOIN2 = 2,
    ZHONGGU_IOIn_IOIN5_TOOL_FIX_BTN = 5,
    ZHONGGU_IOIn_IOIN6_WORK_FIX_BTN = 6,
    ZHONGGU_IOIn_IOIN12_TOOL_EXIST = 12,
    ZHONGGU_IOIn_IOIN13_WORK_EXIST = 13,
    ZHONGGU_IOIn_IOIN14_TOOL_PRESSURE = 14,
    ZHONGGU_IOIn_IOIN15_WORK_PRESSURE = 15,
};
#elif  (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
enum ZHONGGU_IOOut {
    // 407 IO
    ZHONGGU_IOOut_IOOUT1_OPUMP = 1, // 外冲
    ZHONGGU_IOOut_IOOUT2_IPUMP = 2, // 内冲
    ZHONGGU_IOOut_IOOUT3_MACH = 3, // 加工
    ZHONGGU_IOOut_IOOUT4_BZ = 4, // BZ
    ZHONGGU_IOOut_CAP1 = 5, // 电容1
    ZHONGGU_IOOut_CAP2 = 6, // 电容2
    ZHONGGU_IOOut_CAP4 = 7, // 电容4
    ZHONGGU_IOOut_CAP8 = 8, // 电容8

    // 103 IO
    ZHONGGU_IOOut_IOOUT5_LIGHT = 9,
    ZHONGGU_IOOut_IOOUT6_RED = 10,
    ZHONGGU_IOOut_IOOUT7_YELLOW = 11,
    ZHONGGU_IOOut_IOOUT8_GREEN = 12,
    ZHONGGU_IOOut_IOOUT9_TOOL_FIXTURE = 13,
    ZHONGGU_IOOut_IOOUT10_FUSI_IN = 14, // 扶丝入
    ZHONGGU_IOOut_IOOUT11_FUSI_OUT = 15, // 扶丝出
    ZHONGGU_IOOut_IOOUT12_WORK_FIXTURE = 16,

    ZHONGGU_IOOut_TON1 = 17,
    ZHONGGU_IOOut_TON2 = 18,
    ZHONGGU_IOOut_TON4 = 19,
    ZHONGGU_IOOut_TON8 = 20,

    ZHONGGU_IOOut_TOFF1 = 21,
    ZHONGGU_IOOut_TOFF2 = 22,
    ZHONGGU_IOOut_TOFF4 = 23,
    ZHONGGU_IOOut_TOFF8 = 24,

    ZHONGGU_IOOut_IP1 = 25,
    ZHONGGU_IOOut_IP2 = 26,
    ZHONGGU_IOOut_IP4 = 27,
    ZHONGGU_IOOut_IP8 = 28,

    ZHONGGU_IOOut_WORK = 29, // 加工时请置1
    ZHONGGU_IOOut_TOOL = 30, // 正极性时置0, 反极性时置1(TODO, 待测试, 先不开放)
};

enum ZHONGGU_IOIn {
    ZHONGGU_IOIn_IOIN1_TOTAL_PRESSURE = 1, // 总压
    ZHONGGU_IOIn_IOIN2_WATER_PRESSURE = 2, // 水压
    ZHONGGU_IOIn_IOIN3_WATER_QUALITY = 3, // 水质
    ZHONGGU_IOIn_IOIN4_WATER_LEVEL = 4, // 水位
    ZHONGGU_IOIn_IOIN5_TOOL_FIX_BTN = 5, // 松刀按钮
    ZHONGGU_IOIn_IOIN6_PROTECT_DOOR = 6, // 防护门
    ZHONGGU_IOIn_IOIN7_FUSI_IN = 7, // 扶丝收
    ZHONGGU_IOIn_IOIN8_FUSI_OUT = 8, // 扶丝出
    ZHONGGU_IOIn_IOIN9_DIRECTORT_LEFT = 9, // 导向左
    ZHONGGU_IOIn_IOIN10_DIRECTORT_RIGHT = 10, // 导向右
    ZHONGGU_IOIn_IOIN11_DIRECTORT_ZERO = 11, // 原点
    ZHONGGU_IOIn_IOIN12_TOOL_EXIST = 12, // 电极存在
    ZHONGGU_IOIn_IOIN13_WORK_EXIST = 13, // 工件存在
    ZHONGGU_IOIn_IOIN14_TOOL_PRESSURE = 14, // 电极松气压
    ZHONGGU_IOIn_IOIN15_WORK_PRESSURE = 15, // 工件松气压
    ZHONGGU_IOIn_IOIN16_H_WARN = 16, // H报警
    ZHONGGU_IOIn_IOIN17_H_ZERO = 17, // H原点
};
#endif

/* 电参数结构体 */
struct EleParam_dkd_t /* 前缀为__的变量表示尚未用到 */
{
    using ptr = std::shared_ptr<EleParam_dkd_t>;

    /* type */ /* 变量名 */ /* 上位机变量名 */ /* 注释 */ /* 备注 */
    uint32_t __jump_speed; // para_jump_speed;       //抬刀速度-使用 定深
    uint32_t pre_jump_height; // para_pre_jump_height;  //抬刀高度-使用 转速
    uint32_t jump_js;         // para_jump_jerk;        /* 抬刀JS  */

    uint16_t ip; // para_IP;               //IP-4:主电源低压电流峰值
    uint16_t shakemove_LN; // para_LN; //摇动轨迹形状&平面选择&摇动控制   //
                           // 个位形状: 0 OFF, 1 圆, 2 方, 3 菱, 4 X, 5 十
                           // 十位平面: 0 XY , 1 XZ , 2 YZ 无伺服
                           //           3 XY , 4 XZ , 5 YZ 轨迹回退
                           //           6 XY , 7 XZ , 8 YZ 中心回退
                           // 百位摇动控制: 0 自由摇动, 1 H.S摇动, 2 锁定摇动,
                           //               5 象限自由摇动, 6 象限H.S摇动, 7
                           //               象限锁定摇动
    uint16_t JumpHeight; // para_jump_distance;    //设定抬刀高度
    uint16_t
        shakemove_LP; // para_LP;               //LP:象限摇动轨迹的形状 //
                      // 个位十位百位千位分别为: 第一、二、三、四象限摇动形状

    uint8_t UpperThreshold; // para_upper_threshold;   //电压上门限-使用
    uint8_t LowerThreshold; // para_lower_threshold;   //电压下门限-使用
    uint8_t pulse_on;   // para_ON;                //ON-1: 放电脉冲时间
    uint8_t JumpPeriod; // para_jump_interval;     //抬刀间隔 (单位0.1s)

    uint8_t ma; // para_MA;                //MA-3:放电休止时间的调整
    uint8_t lv; // para_V;                 //V-11:主电源电压 //1--90v; 2--120v
                // 不能选择其他值
    uint8_t pp; // para_PP;                //PP-13:PIKADEN脉冲电流波形控制
                // //只能取0,1,2,3这几个数
    uint8_t hp; // para_HP;                //HP-12:辅助电源回路控制

    uint8_t c; // para_C;                 //C-14:极间电容器回路*
    uint8_t servo_speed; /*s*/ // para_ServoVoltage;      //S-15:伺服速度 // s
    uint8_t pl; // para_polarity;          //放电极性 ：0--负极性；1--正极性
    uint8_t al; // para_AL;                //异常放电检验标准

    uint8_t oc; // para_OC;                //ONCUT控制标准
    uint8_t ld; // para_LD;                //ON/OFF控制速度的调整
    uint8_t servo_sensitivity; // para_servo_factor;      //-灵敏度
    uint8_t __servo_rev;       // para_servo_rev;         //-备用

    uint8_t sv;          // para_SV;                //SV-5:伺服基准电压
    uint8_t pulse_off;   // para_OFF;               //PID2 // 脉冲间隔
    uint8_t __strategy;  // para_strategy;          //PID3 加工策略
    uint8_t shakemove_L; // para_L;                 //摇动速度 & 摇动方向
                         // (十进制)    // 个位速度可以取0-9, 0最快, 9最慢;
                         //  十位摇动方向(not used yet), 1 逆时针, 2 顺时针, 0
                         //  每2次回转翻转一次

    uint8_t up;       // para_UP;                //UP-6:跳度上升时间*
    uint8_t dn;       // para_DN;                //DN-7:跳度放电时间
    uint8_t __spare1; // para_spare1;            //PID7
    uint8_t index;    // elec_index;             //PID8
                   // 电参数号(已转换为0-99的编号)

    uint32_t shakemove_STEP; // para_STEP;             // 摇动平动量(半径, um)
                             // // 摇动半径 支持动态更改, 0-9999

    uint16_t upper_index; // 上位机数据库中的条件号, 特殊条件号特殊处理
    uint16_t jump_jm; // 抬刀JM参数
}; /* 表示使用此结构体, 来直接定义电参数格式, 用于接收电参数和存储电参数 */

/* 对io板发送 伺服参数 定义 */
struct CanIOBoardServoSettingStrc {
    // index 0:
    uint8_t servo_voltage_1   : 8; // 伺服参考电压1 //! 目前没用

    // index 1:
    uint8_t servo_voltage_2   : 8; // 伺服参考电压2 //! 目前没用

    // index 2:
    uint8_t servo_sensitivity : 8; // 伺服灵敏度 //! 保持100

    // index 3:
    uint8_t servo_speed       : 8; // 伺服速度 //! 对应 S 0-9

    // index 4:
    uint8_t servo_sv          : 8; // 参考电压 //! 对应 SV 0-9档位, >=10时为实际值

    // index 5:
    uint8_t touch_zigbee_warning_enable : 1; // 接触感知报警蜂鸣器使能, 1 为使能
    uint8_t _reserved_7 : 7;

    // index 6 ~ 7:
    uint8_t _reserved[2]; //  reserved
};
static_assert(sizeof(struct CanIOBoardServoSettingStrc) == 8);


typedef enum {
    // 抬刀结束通知IO板, 伺服速度可以加快 (不用了)
    CommonMessageType_JumpDownNotify = 0, 
    
    // 蜂鸣器响一次
    CommomMessageType_BZOnceNotify = 1,
} CommonMessageType_t ;
/* 不重要的一般性消息 */
struct __attribute__((__packed__)) CanIOBoardCommonMessageStrc {
    // index 0: 
    uint8_t message_type : 8;    // 信息类型

    union __attribute__((__packed__)) {
        //CommonMessageType_JumpDownNotify
        struct __attribute__((__packed__)) {
            uint16_t JumpDownNotify_PreJumpDistance : 16;
            uint16_t JumpDownNotify_PreJumpSpeed : 16;
        };
        
        // CommomMessageType_BZOnceNotify
        uint8_t BZ_Once_flag : 8;
        
        uint8_t data[7]; // 信息数据
    };
};
static_assert(sizeof(struct CanIOBoardCommonMessageStrc) == 8);

/* 对IO板的周期性通信 */
struct CanIOBoardEleParamsStrc {
    // index 0: 
    uint8_t on : 8;    // 脉宽档位
    
    // index 1: 
    uint8_t off : 8;    // 脉间档位
    
    // index 2: 
    uint8_t ip : 8;  // ip档位
    
    // index 3:
    uint8_t pp : 8;   // 高压低压档位
    
    // index 4:
    uint8_t is_finishing_cut : 1; // 精加工标志位
    uint8_t _reserved_7 : 7;  
    
    // index 5 ~ 7:
    uint8_t _reserved[3];           //  reserved
};
static_assert(sizeof(struct CanIOBoardEleParamsStrc) == 8);


/* 通知水泵状态变化 */
struct CanHandboxIOStatus {
    // index 0: 
    uint8_t pump_on : 1;    // 脉宽档位
    uint8_t _reserved1 : 7;    //  reserved
    
    // index 1 ~ 7:
    uint8_t _reserved2[7];           //  reserved
};
static_assert(sizeof(struct CanHandboxIOStatus) == 8);

/* 103返回的小孔机导向器信息 */
struct CanIOBoardDirectorState {
    int32_t curr_step : 32;
    uint8_t director_state : 8;
    uint8_t reserved_1 : 8;
    uint8_t reserved_2 : 8;
    uint8_t reserved_3 : 8;
};
static_assert(sizeof(struct CanIOBoardDirectorState) == 8);

struct CanIOBoardDirectorStartPointMove {
    int32_t target_inc : 32;
    uint16_t speed : 16;
    uint16_t reserved : 16;
};
static_assert(sizeof(struct CanIOBoardDirectorStartPointMove) == 8);
struct CanIOBoardDirectorStopPointMove {
    uint32_t reserved1 : 32;
    uint32_t reserved2 : 32;
};
static_assert(sizeof(struct CanIOBoardDirectorStopPointMove) == 8);
struct CanIOBoardDirectorStartHomeMove {
    uint16_t back_speed : 16;
    uint16_t forward_speed : 16;
    uint32_t reserved : 32;
};
static_assert(sizeof(struct CanIOBoardDirectorStartHomeMove) == 8);

} // namespace power

} // namespace edm
