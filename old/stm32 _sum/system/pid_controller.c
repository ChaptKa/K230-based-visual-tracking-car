#include "pid_controller.h"
#include <stdint.h>

// 使用头文件中定义的结构体，不要重复定义
static PID_Controller_t pid = {0.8f, 0.01f, 0.2f, 0.0f, 0.0f};

void PID_Init(void) {
    pid.Kp = 0.8f;
    pid.Ki = 0.01f;
    pid.Kd = 0.2f;
    pid.prev_error = 0.0f;
    pid.integral = 0.0f;
}

float PID_Calculate(int16_t deviation) {
    float error = (float)deviation;
    
    pid.integral += error;
    if (pid.integral > 100) pid.integral = 100;
    if (pid.integral < -100) pid.integral = -100;
    
    float derivative = error - pid.prev_error;
    pid.prev_error = error;
    
    float output = pid.Kp * error + pid.Ki * pid.integral + pid.Kd * derivative;
    
    if (output > 100) output = 100;
    if (output < -100) output = -100;
    
    return output;
}

void PID_SetParams(float kp, float ki, float kd) {
    pid.Kp = kp;
    pid.Ki = ki;
    pid.Kd = kd;
}