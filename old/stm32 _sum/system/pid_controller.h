#ifndef __PID_CONTROLLER_H
#define __PID_CONTROLLER_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

typedef struct {
    float Kp;
    float Ki; 
    float Kd;
    float prev_error;
    float integral;
} PID_Controller_t;

// 函数声明
void PID_Init(void);
float PID_Calculate(int16_t deviation);
void PID_SetParams(float kp, float ki, float kd);

#ifdef __cplusplus
}
#endif

#endif /* __PID_CONTROLLER_H */
