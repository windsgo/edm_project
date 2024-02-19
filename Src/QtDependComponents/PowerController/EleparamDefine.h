#pragma once

#include <cstdint>
#include <memory>

namespace edm
{

namespace power
{

#define DEC_LV0_CONTACTORS 0
#define DEC_LV1_CONTACTORS 2
#define DEC_LV2_CONTACTORS 3

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

/* 继电器开关定义 */
#define CONTACTOR_DISABLE       0 // 断开继电器
#define CONTACTOR_ENABLE        1 // 吸合继电器

// CanMachineIO 如果对应继电器, 对应位置1, 表示继电器吸合(晶体管输出使能); 为0则表示继电器断开

/* 电参数结构体 */
struct EleParam_dkd_t /* 前缀为__的变量表示尚未用到 */
{
    using ptr = std::shared_ptr<EleParam_dkd_t>;

    /* type */  /* 变量名 */        /* 上位机变量名 */        /* 注释 */                         /* 备注 */
    uint32_t    __jump_speed;         // para_jump_speed;       //抬刀速度-使用 定深
    uint32_t    pre_jump_height;    // para_pre_jump_height;  //抬刀高度-使用 转速
    uint32_t    jump_js;            // para_jump_jerk;        /* 抬刀JS  */

    uint16_t    ip;                 // para_IP;               //IP-4:主电源低压电流峰值
    uint16_t    shakemove_LN;       // para_LN;               //摇动轨迹形状&平面选择&摇动控制   // 个位形状: 0 OFF, 1 圆, 2 方, 3 菱, 4 X, 5 十
                                                                                                 // 十位平面: 0 XY , 1 XZ , 2 YZ 无伺服
                                                                                                 //           3 XY , 4 XZ , 5 YZ 轨迹回退
                                                                                                 //           6 XY , 7 XZ , 8 YZ 中心回退
                                                                                                 // 百位摇动控制: 0 自由摇动, 1 H.S摇动, 2 锁定摇动, 
                                                                                                 //               5 象限自由摇动, 6 象限H.S摇动, 7 象限锁定摇动
    uint16_t    JumpHeight;         // para_jump_distance;    //设定抬刀高度
    uint16_t    shakemove_LP;       // para_LP;               //LP:象限摇动轨迹的形状            // 个位十位百位千位分别为: 第一、二、三、四象限摇动形状
                                    
    uint8_t     UpperThreshold;     //para_upper_threshold;   //电压上门限-使用
    uint8_t     LowerThreshold;     //para_lower_threshold;   //电压下门限-使用
    uint8_t     pulse_on;           //para_ON;                //ON-1: 放电脉冲时间
    uint8_t     JumpPeriod;         //para_jump_interval;     //抬刀间隔 (单位0.1s)
                                    
    uint8_t     ma;                 //para_MA;                //MA-3:放电休止时间的调整
    uint8_t     lv;                 //para_V;                 //V-11:主电源电压                  //1--90v; 2--120v  不能选择其他值
    uint8_t     pp;                 //para_PP;                //PP-13:PIKADEN脉冲电流波形控制    //只能取0,1,2,3这几个数
    uint8_t     hp;                 //para_HP;                //HP-12:辅助电源回路控制
                                    
    uint8_t     c;                  //para_C;                 //C-14:极间电容器回路*
    uint8_t     servo_speed;/*s*/   //para_ServoVoltage;      //S-15:伺服速度 // s
    uint8_t     pl;                 //para_polarity;          //放电极性 ：0--负极性；1--正极性
    uint8_t     al;                 //para_AL;                //异常放电检验标准
                                    
    uint8_t     oc;                 //para_OC;                //ONCUT控制标准
    uint8_t     ld;                 //para_LD;                //ON/OFF控制速度的调整
    uint8_t     servo_sensitivity;  //para_servo_factor;      //-灵敏度
    uint8_t     __servo_rev;        //para_servo_rev;         //-备用
    
    uint8_t     sv;                 //para_SV;                //SV-5:伺服基准电压
    uint8_t     pulse_off;          //para_OFF;               //PID2                            // 脉冲间隔
    uint8_t     __strategy;         //para_strategy;          //PID3 加工策略
    uint8_t     shakemove_L;        //para_L;                 //摇动速度 & 摇动方向 (十进制)    // 个位速度可以取0-9, 0最快, 9最慢; 
                                                                                                // 十位摇动方向(not used yet), 1 逆时针, 2 顺时针, 0 每2次回转翻转一次
    
    uint8_t     up;                 //para_UP;                //UP-6:跳度上升时间*
    uint8_t     dn;                 //para_DN;                //DN-7:跳度放电时间
    uint8_t     __spare1;           //para_spare1;            //PID7
    uint8_t     index;              //elec_index;             //PID8 电参数号(已转换为0-99的编号)
    
    uint32_t    shakemove_STEP;     // para_STEP;             // 摇动平动量(半径, um)           // 摇动半径 支持动态更改, 0-9999
    
    uint16_t    upper_index;                                  // 上位机数据库中的条件号, 特殊条件号特殊处理
    uint16_t    jump_jm;                                      // 抬刀JM参数
}; /* 表示使用此结构体, 来直接定义电参数格式, 用于接收电参数和存储电参数 */

/* 对io板发送 伺服参数 定义 */
struct CanIOBoardServoSettingStrc {
    // index 0: 
    uint8_t servo_voltage_1 : 8;    // 伺服参考电压1
    
    // index 1: 
    uint8_t servo_voltage_2 : 8;    // 伺服参考电压2
    
    // index 2: 
    uint8_t servo_sensitivity : 8;  // 伺服灵敏度
    
    // index 3:
    uint8_t servo_speed : 8;        // 伺服速度
    
    // index 4:
    uint8_t servo_sv : 8;           // 参考电压
    
    // index 5:
    uint8_t touch_zigbee_warning_enable : 1; // 接触感知报警蜂鸣器使能, 1 为使能
    uint8_t _reserved_7 : 7;    
    
    // index 6 ~ 7:
    uint8_t _reserved[2];           //  reserved
};
static_assert(sizeof(struct CanIOBoardServoSettingStrc) == 8);

} // namespace power

} // namespace edm
