#ifndef __PROJECT_CONFIG_H
#define __PROJECT_CONFIG_H

#include <stdint.h>

// 系统状态定义
typedef enum {
    STATE_IDLE,
    STATE_TASK1_RUNNING,
    STATE_TASK2_RUNNING, 
    STATE_TASK3_RUNNING,
    STATE_TURNING,
    STATE_STRAIGHT_RUNNING,
    STATE_FINISH,
    STATE_TIMEOUT
} SystemState_t;

// 任务步骤定义
typedef enum {
    STEP0,
    STEP1,
    STEP2,
    STEP3,
    STEP4,
    STEP5,
    STEP6,
    STEP7,
    STEP8,
    STEP9
} TaskStep_t;

// 视觉数据结构
typedef struct {
    int16_t deviation;
    uint8_t mode;
    uint8_t data_valid;
} VisionData_t;

// 全局变量声明
extern SystemState_t current_state;
extern TaskStep_t current_task_step;
extern VisionData_t vision_data;

#endif
