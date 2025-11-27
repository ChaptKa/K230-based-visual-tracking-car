#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f10x.h"

void Delay_us(uint32_t us);
void Delay_ms(uint32_t ms);
void Delay_s(uint32_t s);

// 添加系统时间函数
void SysTime_Init(void);
uint32_t GetSystemTime(void);  // 返回毫秒时间

#endif
