#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Motor1.h"
#include "Drive.h"
#include "Control.h"
#include <stdio.h>

#include "vision.h"
#include "state_machine.h"
#include "pid_controller.h"
#include "motor_control.h"
#include "led.h"
#include "buzzer.h"

int main(void) {
    // 系统初始化
    SysTime_Init();
    
    motor_pwm_init();
    Drive_Init();
    
    Vision_Init();
    StateMachine_Init();
    PID_Init();
    MotorControl_Init();
    LED_Init();
    Buzzer_Init();
    
    printf("System initialized, starting main loop...\r\n");
    
    // 基础功能测试
    LED_SetState(LED_ON);
    Delay_ms(1000);
    LED_SetState(LED_BLINK);
    Delay_ms(1000);
    Buzzer_TimeoutAlert();
    
    while(1) {
        MainControlLoop();
        LED_Update();
        Delay_ms(10);
    }
}