#ifndef __VISION_H
#define __VISION_H

#include "project_config.h"

// 函数声明
void Vision_Init(void);
void ParseVisionData(uint8_t* raw_data);
VisionData_t GetVisionData(void);

// K210通信函数
void Send_StartSignal(void);
void Send_TaskStatus(uint8_t task_id);
void Send_FinishSignal(void);
void Send_CustomCommand(const char* command);
void Test_UART_Communication(void);

#endif
