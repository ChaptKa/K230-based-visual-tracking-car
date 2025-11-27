#ifndef FUNCTION_DECLARATIONS_H
#define FUNCTION_DECLARATIONS_H

#include <stdint.h>

// 驱动函数声明
void Drive_Init(void);
void Left_run(void);
void Right_run(void);
void Left_back(void);
void Right_back(void);
void Turn_right(void);
void Turn_left(void);
void Run(void);
void Step(void);

// PWM 函数声明
void PWM_SetCompare3(uint32_t value);
void PWM_SetCompare4(uint32_t value);

// 任务处理函数声明
void HandlePathDecision(void);
void HandleTask1(void);
void HandleTask2(void);
void HandleTask3(void);
void StartStraightRun(uint32_t duration);

#endif
