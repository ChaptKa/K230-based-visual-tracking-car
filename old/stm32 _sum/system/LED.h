#ifndef __LED_H
#define __LED_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f10x.h"

// LED状态枚举定义
typedef enum {
    LED_OFF = 0,
    LED_ON = 1,
    LED_BLINK = 2
} LED_State_t;

// 函数声明
void LED_Init(void);
void LED_SetState(LED_State_t state);
void LED_Update(void);
LED_State_t LED_GetState(void);  // 添加获取状态函数

#ifdef __cplusplus
}
#endif

#endif /* __LED_H */
