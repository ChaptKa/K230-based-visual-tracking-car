#include "stm32f10x.h"
#include "project_config.h"
#include "control.h"
#include "vision.h"
#include "pid_controller.h"
#include "state_machine.h"
#include "Drive.h"
#include "Delay.h"
#include <stdio.h>

// ==================== 函数声明 ====================
// 运动控制函数声明
void Run(void);
void Back(void);
void Turn_left(void);
void Turn_right(void);
void Step(void);

// 内部函数声明
static void HandlePathDecision(void);
static void HandleTask1(void);
static void HandleTask2(void);
static void HandleTask3(void);
static void StartStraightRun(uint32_t duration);

// 全局变量定义
SystemState_t current_state = STATE_IDLE;
TaskStep_t current_task_step = STEP0;
VisionData_t vision_data = {0, 0, 0};

// 控制状态变量
static uint32_t last_control_time = 0;
static uint32_t straight_start_time = 0;
static uint32_t straight_duration = 1000;
static uint8_t is_straight_running = 0;

// ==================== 主控制循环 ====================
void MainControlLoop(void) {
    uint32_t current_time = GetSystemTime();
    
    if(current_time - last_control_time < 20) {
        return;
    }
    last_control_time = current_time;
    
    if(vision_data.data_valid) {
        // 基础循迹控制
        float pid_output = PID_Calculate(vision_data.deviation);
        
        // 根据PID输出调整电机速度
        int base_speed = 700;
        int left_speed = base_speed + (int)pid_output;
        int right_speed = base_speed - (int)pid_output;
        
        // 限制速度范围
        if(left_speed > 1000) left_speed = 1000;
        if(left_speed < 400) left_speed = 400;
        if(right_speed > 1000) right_speed = 1000;
        if(right_speed < 400) right_speed = 400;
        
        PWM_SetCompare3(left_speed);
        PWM_SetCompare4(right_speed);
        Left_run();
        Right_run();
        
        // 处理路径决策
        HandlePathDecision();
    } else {
        Step();
    }
    
    StateMachine_Update();
}

// ==================== 路径决策函数 ====================
static void HandlePathDecision(void) {
    uint32_t current_time = GetSystemTime();
    
    if(current_state == STATE_STRAIGHT_RUNNING) {
        if(current_time - straight_start_time > straight_duration) {
            is_straight_running = 0;
            Step();
            current_task_step++;
        }
        return;
    }
    
    switch(current_state) {
        case STATE_TASK1_RUNNING:
            HandleTask1();
            break;
        case STATE_TASK2_RUNNING:
            HandleTask2();
            break;
        case STATE_TASK3_RUNNING:
            HandleTask3();
            break;
        default:
            break;
    }
}

// ==================== 任务处理函数 ====================
static void HandleTask1(void) {
    switch(current_task_step) {
        case STEP0:
            if(vision_data.mode == 2) {
                Turn_right();
                Delay_ms(500);
                Run();
                current_task_step = STEP1;
            }
            break;
        case STEP1:
            if(vision_data.mode == 3) {
                Turn_left();
                Delay_ms(500);
                Run();
                current_task_step = STEP2;
            }
            break;
        case STEP2:
            StartStraightRun(1000);
            break;
        case STEP3:
            Turn_right();
            Delay_ms(500);
            Run();
            current_task_step = STEP4;
            break;
        case STEP4:
            current_state = STATE_TASK2_RUNNING;
            current_task_step = STEP0;
            break;
    }
}

static void HandleTask2(void) {
    switch(current_task_step) {
        case STEP0:
            StartStraightRun(2000);
            break;
        case STEP1:
            Turn_right();
            Delay_ms(500);
            Run();
            current_task_step = STEP2;
            break;
        case STEP2:
            StartStraightRun(2000);
            break;
        case STEP3:
            Turn_left();
            Delay_ms(500);
            Run();
            current_task_step = STEP4;
            break;
        case STEP4:
            StartStraightRun(2000);
            break;
        case STEP5:
            Turn_right();
            Delay_ms(500);
            Run();
            current_task_step = STEP6;
            break;
        case STEP6:
            StartStraightRun(2000);
            break;
        case STEP7:
            current_state = STATE_TASK3_RUNNING;
            current_task_step = STEP0;
            break;
    }
}

static void HandleTask3(void) {
    switch(current_task_step) {
        case STEP0:
            if(vision_data.mode == 2) {
                Turn_right();
                Delay_ms(500);
                Run();
                current_task_step = STEP1;
            }
            break;
        case STEP1:
            StartStraightRun(2000);
            break;
        case STEP2:
            Turn_right();
            Delay_ms(500);
            Run();
            current_task_step = STEP3;
            break;
        case STEP3:
            StartStraightRun(1000);
            break;
        case STEP4:
            Turn_left();
            Delay_ms(500);
            Run();
            current_task_step = STEP5;
            break;
        case STEP5:
            if(vision_data.mode == 1) {
                Turn_left();
                Delay_ms(500);
                Run();
                current_task_step = STEP6;
            }
            break;
        case STEP6:
            StartStraightRun(2000);
            break;
        case STEP7:
            Turn_right();
            Delay_ms(500);
            Run();
            current_task_step = STEP8;
            break;
        case STEP8:
            StartStraightRun(1000);
            break;
        case STEP9:
            Step();
            current_state = STATE_FINISH;
            break;
    }
}

static void StartStraightRun(uint32_t duration) {
    if(!is_straight_running) {
        straight_start_time = GetSystemTime();
        straight_duration = duration;
        is_straight_running = 1;
        current_state = STATE_STRAIGHT_RUNNING;
    }
}

// ==================== 运动控制函数实现 ====================
void Run(void) {	
	PWM_SetCompare3(900);
	PWM_SetCompare4(900);
	Left_run();
	Right_run();	
}

void Back(void) {
	PWM_SetCompare3(500);
	PWM_SetCompare4(500);
	Left_back();
	Right_back();	
}

void Turn_left(void) {
	PWM_SetCompare3(400);
	PWM_SetCompare4(400);
	Left_back();
	Right_run();
}

void Turn_right(void) {
	PWM_SetCompare3(400);
	PWM_SetCompare4(400);
	Right_back();
	Left_run();
}	

void Step(void) {
    Left_step();
    Right_step();
}