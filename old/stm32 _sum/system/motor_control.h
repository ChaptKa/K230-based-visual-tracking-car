#ifndef __MOTOR_CONTROL_H
#define __MOTOR_CONTROL_H

#include "stm32f10x.h"

void MotorControl_Init(void);
void Motor_Control(float pid_output);
void HandleIntersection(uint8_t mode);
void MainControlLoop(void);

// 基础电机控制函数
void Motor_Left(int16_t speed);
void Motor_Right(int16_t speed);
void Motor_GoForward(uint16_t speed);
void Motor_TurnLeft(uint16_t speed);
void Motor_TurnRight(uint16_t speed);
void Motor_Stop(void);

#endif
