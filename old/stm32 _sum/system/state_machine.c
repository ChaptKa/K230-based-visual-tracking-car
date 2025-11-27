#include "stm32f10x.h"
#include "project_config.h"
#include "state_machine.h"
#include "led.h"
#include "buzzer.h"
#include "Delay.h"
#include "vision.h"
#include <stdio.h>

static uint32_t task_start_time = 0;
static uint8_t start_signal_sent = 0;  // 标记启动信号是否已发送

void StateMachine_Init(void) {
    current_state = STATE_IDLE;
    current_task_step = STEP0;
    task_start_time = 0;
    start_signal_sent = 0;
    
    printf("State Machine Initialized\r\n");
}

void StateMachine_Update(void) {
    uint32_t current_time = GetSystemTime();
    
    switch(current_state) {
        case STATE_IDLE:
            current_state = STATE_TASK1_RUNNING;
            task_start_time = current_time;
            current_task_step = STEP0;
            LED_SetState(LED_OFF);
            
            // 发送启动信号给K210（只发送一次）
            if (!start_signal_sent) {
                Send_StartSignal();
                Delay_ms(100);  // 给K210一些响应时间
                Send_TaskStatus(1);  // 任务1开始
                start_signal_sent = 1;
                printf("=== Starting Task 1 === Signals sent to K210\r\n");
            }
            break;
            
        case STATE_TASK1_RUNNING:
            if (current_time - task_start_time > 20000) {
                current_state = STATE_TIMEOUT;
                printf("!!! Task 1 Timeout !!!\r\n");
                Buzzer_TimeoutAlert();
                Step();
            } else if (CheckTask1Complete()) {
                LED_SetState(LED_ON);
                Send_TaskStatus(2);  // 任务2开始
                printf("=== Task 1 Completed === Starting Task 2\r\n");
            }
            break;
            
        case STATE_TASK2_RUNNING:
            if (current_time - task_start_time > 40000) {
                current_state = STATE_TIMEOUT;
                printf("!!! Task 2 Timeout !!!\r\n");
                Buzzer_TimeoutAlert();
                Step();
            } else if (CheckTask2Complete()) {
                LED_SetState(LED_BLINK);
                Send_TaskStatus(3);  // 任务3开始
                printf("=== Task 2 Completed === Starting Task 3\r\n");
            }
            break;
            
        case STATE_TASK3_RUNNING:
            if (current_time - task_start_time > 40000) {
                current_state = STATE_TIMEOUT;
                printf("!!! Task 3 Timeout !!!\r\n");
                Buzzer_TimeoutAlert();
                Step();
            } else if (CheckTask3Complete()) {
                LED_SetState(LED_OFF);
                Send_FinishSignal();  // 所有任务完成
                printf("=== All Tasks Completed === Finish signal sent to K210\r\n");
            }
            break;
            
        case STATE_FINISH:
            Step();
            // 可以在这里添加完成后的处理
            break;
            
        case STATE_TIMEOUT:
            Step();
            Buzzer_TimeoutAlert();
            // 超时状态下可以发送错误信号给K210
            printf("!!! System Timeout !!!\r\n");
            break;
            
        default:
            break;
    }
    
    // 调试信息输出（可选）
    static uint32_t last_debug_time = 0;
    if (current_time - last_debug_time > 1000) {  // 每秒输出一次状态
        last_debug_time = current_time;
        printf("State: %d, Step: %d, Time: %lu\r\n", 
               current_state, current_task_step, current_time - task_start_time);
    }
}

uint8_t CheckTask1Complete(void) {
    // 任务1完成条件：到达STEP4并且状态已切换到TASK2
    uint8_t completed = (current_task_step >= STEP4 && current_state == STATE_TASK2_RUNNING);
    if (completed) {
        printf("CheckTask1Complete: TRUE (Step=%d, State=%d)\r\n", current_task_step, current_state);
    }
    return completed;
}

uint8_t CheckTask2Complete(void) {
    // 任务2完成条件：到达STEP7并且状态已切换到TASK3
    uint8_t completed = (current_task_step >= STEP7 && current_state == STATE_TASK3_RUNNING);
    if (completed) {
        printf("CheckTask2Complete: TRUE (Step=%d, State=%d)\r\n", current_task_step, current_state);
    }
    return completed;
}

uint8_t CheckTask3Complete(void) {
    // 任务3完成条件：到达STEP9并且状态为FINISH
    uint8_t completed = (current_task_step >= STEP9 && current_state == STATE_FINISH);
    if (completed) {
        printf("CheckTask3Complete: TRUE (Step=%d, State=%d)\r\n", current_task_step, current_state);
    }
    return completed;
}

// 重置状态机（可选功能）
void StateMachine_Reset(void) {
    current_state = STATE_IDLE;
    current_task_step = STEP0;
    task_start_time = 0;
    start_signal_sent = 0;
    printf("State Machine Reset\r\n");
}

// 获取当前任务运行时间（可选功能）
uint32_t GetTaskRunningTime(void) {
    if (current_state == STATE_IDLE) {
        return 0;
    }
    return GetSystemTime() - task_start_time;
}

// 获取剩余时间（可选功能）
uint32_t GetTaskRemainingTime(void) {
    uint32_t elapsed = GetTaskRunningTime();
    uint32_t limit = 0;
    
    switch(current_state) {
        case STATE_TASK1_RUNNING:
            limit = 20000;  // 20秒
            break;
        case STATE_TASK2_RUNNING:
            limit = 40000;  // 40秒
            break;
        case STATE_TASK3_RUNNING:
            limit = 40000;  // 40秒
            break;
        default:
            return 0;
    }
    
    if (elapsed >= limit) {
        return 0;
    }
    return limit - elapsed;
}