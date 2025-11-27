#ifndef __CONTROL_H
#define __CONTROL_H

#include "project_config.h"

// 运动控制函数声明
void Run(void);
void Back(void);
void Turn_left(void);
void Turn_right(void);
void Step(void);

// 主控制循环
void MainControlLoop(void);

#endif
