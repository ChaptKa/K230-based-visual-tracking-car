#include "stm32f10x.h"                  // Device header
// b11/4 b10/3 a8 a9
void motor_pin_init()
{
	GPIO_InitTypeDef initstruct;
	initstruct.GPIO_Mode=GPIO_Mode_AF_PP;
	initstruct.GPIO_Pin=GPIO_Pin_2|GPIO_Pin_3;
	initstruct.GPIO_Speed=GPIO_Speed_50MHz;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	GPIO_Init(GPIOA,&initstruct);
	initstruct.GPIO_Mode=GPIO_Mode_Out_PP;
	initstruct.GPIO_Pin=GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10;
	initstruct.GPIO_Speed=GPIO_Speed_50MHz;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	GPIO_Init(GPIOA,&initstruct);
}
void motor_pwm_init()
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM_OCInitTypeDef ocinitstruct;
TIM_TimeBaseInitTypeDef time2struct;
RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
time2struct.TIM_ClockDivision=TIM_CKD_DIV1;
time2struct.TIM_CounterMode=TIM_CounterMode_Up;
time2struct.TIM_Period=200;
time2struct.TIM_Prescaler=7200-1;	
//pwm	
ocinitstruct.TIM_OCMode=TIM_OCMode_PWM1;
ocinitstruct.TIM_OCPolarity=TIM_OCPolarity_High;
ocinitstruct.TIM_OutputState=TIM_OutputNState_Enable;
TIM_OC3Init(TIM2,&ocinitstruct);	
TIM_OC4Init(TIM2,&ocinitstruct);
TIM_Cmd(TIM2,ENABLE);
TIM_OC3PreloadConfig(TIM2,TIM_OCPreload_Enable);
TIM_ARRPreloadConfig(TIM2,ENABLE);
}