#ifndef __STATE_MACHINE_H
#define __STATE_MACHINE_H

#include "project_config.h"

// 函数声明
void StateMachine_Init(void);
void StateMachine_Update(void);
void StateMachine_Reset(void);           // 新增
uint32_t GetTaskRunningTime(void);       // 新增
uint32_t GetTaskRemainingTime(void);     // 新增

uint8_t CheckTask1Complete(void);
uint8_t CheckTask2Complete(void);
uint8_t CheckTask3Complete(void);

#endif
