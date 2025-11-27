#include "motor_control.h"
#include "Drive.h"          // A同学的电机方向控制
#include "Motor1.h"         // A同学的PWM速度控制
#include "Control.h"        // A同学的整体控制
#include "Delay.h"          // A同学的延时函数
#include "vision.h"         // B同学的视觉数据
#include "state_machine.h"  // B同学的状态机
#include "pid_controller.h" // B同学的PID控制

// 声明全局变量（在main.c中定义）
extern SystemState_t current_state;
extern VisionData_t vision_data;

// 电机速度参数
#define BASE_SPEED 600      // 基础PWM值 (根据实际调整)
#define MAX_SPEED 1000      // 最大PWM值
#define MIN_SPEED 300       // 最小PWM值

void MotorControl_Init(void) {
    // 初始化PWM
    motor_pwm_init();
    
    // 初始化电机驱动
    Drive_Init();
    
    // 初始停止状态
    Left_step();
    Right_step();
}

// 设置左电机速度和方向
void Motor_Left(int16_t speed) {
    if (speed > 0) {
        Left_run();  // 正转
        PWM_SetCompare3((uint16_t)speed);
    } else if (speed < 0) {
        Left_back(); // 反转
        PWM_SetCompare3((uint16_t)(-speed));
    } else {
        Left_step(); // 停止
        PWM_SetCompare3(0);
    }
}

// 设置右电机速度和方向
void Motor_Right(int16_t speed) {
    if (speed > 0) {
        Right_run();  // 正转
        PWM_SetCompare4((uint16_t)speed);
    } else if (speed < 0) {
        Right_back(); // 反转
        PWM_SetCompare4((uint16_t)(-speed));
    } else {
        Right_step(); // 停止
        PWM_SetCompare4(0);
    }
}

// PID控制的电机调整
void Motor_Control(float pid_output) {
    int16_t left_speed, right_speed;
    
    // 基础速度
    left_speed = BASE_SPEED;
    right_speed = BASE_SPEED;
    
    // 根据PID输出调整左右轮速度差
    if (pid_output > 0) {
        // 需要左转 - 右轮加速，左轮减速
        right_speed += (int16_t)(pid_output * 2);
        left_speed -= (int16_t)(pid_output * 2);
    } else {
        // 需要右转 - 左轮加速，右轮减速
        left_speed -= (int16_t)(pid_output * 2);  // pid_output为负
        right_speed += (int16_t)(pid_output * 2);
    }
    
    // 速度限制
    if (left_speed > MAX_SPEED) left_speed = MAX_SPEED;
    if (left_speed < MIN_SPEED) left_speed = MIN_SPEED;
    if (right_speed > MAX_SPEED) right_speed = MAX_SPEED;
    if (right_speed < MIN_SPEED) right_speed = MIN_SPEED;
    
    // 设置电机速度
    Motor_Left(left_speed);
    Motor_Right(right_speed);
}

// 基础移动函数（使用PWM控制）
void Motor_GoForward(uint16_t speed) {
    Motor_Left(speed);
    Motor_Right(speed);
}

void Motor_TurnLeft(uint16_t speed) {
    Motor_Left(-speed/2);  // 左轮反转
    Motor_Right(speed);    // 右轮正转
}

void Motor_TurnRight(uint16_t speed) {
    Motor_Left(speed);     // 左轮正转
    Motor_Right(-speed/2); // 右轮反转
}

void Motor_Stop(void) {
    Motor_Left(0);
    Motor_Right(0);
}

void HandleIntersection(uint8_t mode) {
    switch(mode) {
        case 0: // 直行模式 - 正常PID循迹
            // 不需要特殊处理，MainControlLoop会处理
            break;
            
        case 1: // 左岔路
            if (current_state == STATE_TASK2_RUNNING) {
                // 任务2可能需要左转
                printf("Left intersection detected, turning left...\r\n");
                Motor_TurnLeft(400);
                Delay_ms(300);  // 转弯时间
                Motor_GoForward(BASE_SPEED);  // 转弯后继续前进
            } else {
                // 其他情况直行通过
                Motor_GoForward(BASE_SPEED);
                Delay_ms(200);
            }
            break;
            
        case 2: // 右岔路
            if (current_state == STATE_TASK1_RUNNING) {
                // 任务1可能需要右转
                printf("Right intersection detected, turning right...\r\n");
                Motor_TurnRight(400);
                Delay_ms(300);
                Motor_GoForward(BASE_SPEED);
            } else {
                Motor_GoForward(BASE_SPEED);
                Delay_ms(200);
            }
            break;
            
        case 3: // 十字路口
            printf("Cross intersection detected...\r\n");
            // 根据任务需求选择方向
            if (current_state == STATE_TASK3_RUNNING) {
                // 任务3的特定处理 - 直行通过
                Motor_GoForward(BASE_SPEED);
                Delay_ms(500);  // 较长距离通过路口
            } else {
                Motor_GoForward(BASE_SPEED);
                Delay_ms(300);
            }
            break;
    }
}

void MotorControlLoop(void) {
    // 1. 接收并处理视觉数据
    if (vision_data.data_valid) {
        printf("Vision data: deviation=%d, mode=%d\r\n", 
               vision_data.deviation, vision_data.mode);
        
        // 2. 路口决策
        if (vision_data.mode != 0) {  // 非直行模式，说明检测到路口
            HandleIntersection(vision_data.mode);
        } else {
            // 3. PID循迹控制 (只在直行模式下)
            float steer = PID_Calculate(vision_data.deviation);
            Motor_Control(steer);
        }
        
        vision_data.data_valid = 0;  // 清除有效标志
    }
    
    // 4. 状态机更新
    StateMachine_Update();
}