#pragma once

/* 松下驱动器定义开始 */

/* Control Word Definitions Start */
#define CW_Shutdown                 0x6
#define CW_SwitchOn                 0x7
#define CW_EnableOperation          0xF
#define CW_DisableVoltage           0x0
#define CW_QuickStop                0x2
#define CW_DisableOperation         0x7
#define CW_FaultReset               (1 << 7)
/* Control Word Definitions End */

/* Modes of Operation Definitions Start */
#define OM_CSP                      8
/* Modes of Operation Definitions End */

/* Status Word Definitions Start */

// PDS states
#define SW_NotReadyToSwitchOn           0x0
// #define is_SW_NotReadyToSwitchOn(sw)    ((0b01001111 & sw) == SW_NotReadyToSwitchOn)
#define is_SW_NotReadyToSwitchOn(sw)    ((0x4F & sw) == SW_NotReadyToSwitchOn)

#define SW_SwitchOnDisabled             0x40
// #define is_SW_SwitchOnDisabled(sw)      ((0b01001111 & sw) == SW_SwitchOnDisabled)
#define is_SW_SwitchOnDisabled(sw)      ((0x4F & sw) == SW_SwitchOnDisabled)

#define SW_ReadyToSwitchOn              0x21
// #define is_SW_ReadyToSwitchOn(sw)       ((0b01101111 & sw) == SW_ReadyToSwitchOn)
#define is_SW_ReadyToSwitchOn(sw)       ((0x6F & sw) == SW_ReadyToSwitchOn)

#define SW_SwitchedOn                   0x23
// #define is_SW_SwitchedOn(sw)            ((0b01101111 & sw) == SW_SwitchedOn)
#define is_SW_SwitchedOn(sw)            ((0x6F & sw) == SW_SwitchedOn)

#define SW_OperationEnabled             0x27
// #define is_SW_OperationEnabled(sw)      ((0b01101111 & sw) == SW_OperationEnabled)
#define is_SW_OperationEnabled(sw)      ((0x6F & sw) == SW_OperationEnabled)

#define SW_QuickStopActive              0x7
// #define is_SW_QuickStopActive(sw)       ((0b01101111 & sw) == SW_QuickStopActive)
#define is_SW_QuickStopActive(sw)       ((0x6F & sw) == SW_QuickStopActive)

#define SW_FaultReactionActive          0xF
// #define is_SW_FaultReactionActive(sw)   ((0b01001111 & sw) == SW_FaultReactionActive)
#define is_SW_FaultReactionActive(sw)   ((0x4F & sw) == SW_FaultReactionActive)

#define SW_Fault                        0x8
// #define is_SW_Fault(sw)                 ((0b01001111 & sw) == SW_Fault)
#define is_SW_Fault(sw)                 ((0x4F & sw) == SW_Fault)
/* Status Word Definitions End */

/* 松下驱动器定义结束 */