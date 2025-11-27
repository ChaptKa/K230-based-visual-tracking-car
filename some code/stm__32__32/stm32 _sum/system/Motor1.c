#include "stm32f10x.h"                  // Device header
//a2 a3 pwm  a45 a67 µç»ú

void motor_pwm_init()
{	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	
	GPIO_InitTypeDef initstruct;
	initstruct.GPIO_Mode=GPIO_Mode_AF_PP;
	initstruct.GPIO_Pin=GPIO_Pin_2|GPIO_Pin_3;
	initstruct.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&initstruct);
	
	TIM_TimeBaseInitTypeDef time2struct;
	time2struct.TIM_ClockDivision=0;
	time2struct.TIM_CounterMode=TIM_CounterMode_Up;
	time2struct.TIM_Period=1000-1;
	time2struct.TIM_Prescaler=72-1;	
	TIM_TimeBaseInit(TIM2,&time2struct);
	TIM_InternalClockConfig(TIM2);
//pwm
	TIM_OCInitTypeDef ocinitstruct;	
	ocinitstruct.TIM_OCMode=TIM_OCMode_PWM1;
	ocinitstruct.TIM_OCPolarity=TIM_OCPolarity_High;
	ocinitstruct.TIM_OutputState=TIM_OutputState_Enable;
	ocinitstruct.TIM_Pulse=500;	
	TIM_OC3Init(TIM2,&ocinitstruct);
	
	ocinitstruct.TIM_Pulse=500;	
	TIM_OC4Init(TIM2,&ocinitstruct);

	
	TIM_OC3PreloadConfig(TIM2,TIM_OCPreload_Enable);
	TIM_OC4PreloadConfig(TIM2,TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM2,ENABLE);
	TIM_Cmd(TIM2,ENABLE);
	
}
void PWM_SetCompare3(uint16_t Compare)
{
	TIM_SetCompare3(TIM2,Compare);
}
void PWM_SetCompare4(uint16_t Compare)
{
	TIM_SetCompare4(TIM2,Compare);
}