#include "Delay.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"
#include "misc.h"

// 系统时间变量
static volatile uint32_t system_time_ms = 0;

/**
  * @brief  系统时间初始化
  * @param  无
  * @retval 无
  */
void SysTime_Init(void) {
    // 配置TIM2定时器为1ms中断
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    
    // 定时器时钟为72MHz，分频后为72MHz/7200 = 10kHz
    // 计数值10000，所以频率为10kHz/10000 = 1Hz（1ms）
    TIM_TimeBaseStructure.TIM_Period = 10000 - 1;          // 自动重装载值
    TIM_TimeBaseStructure.TIM_Prescaler = 7200 - 1;        // 预分频值
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    // 使能TIM2更新中断
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    
    // 配置NVIC
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 启动定时器
    TIM_Cmd(TIM2, ENABLE);
    
    system_time_ms = 0;
}

/**
  * @brief  获取系统时间（毫秒）
  * @param  无
  * @retval 系统运行时间（毫秒）
  */
uint32_t GetSystemTime(void) {
    return system_time_ms;
}

/**
  * @brief  TIM2中断处理函数
  * @param  无
  * @retval 无
  */
void TIM2_IRQHandler(void) {
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        system_time_ms++;
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}

// 原有的延时函数保持不变
void Delay_us(uint32_t xus)
{
	SysTick->LOAD = 72 * xus;				//设置定时器重装值
	SysTick->VAL = 0x00;					//清空当前计数值
	SysTick->CTRL = 0x00000005;				//设置时钟源为HCLK，启动定时器
	while(!(SysTick->CTRL & 0x00010000));	//等待计数到0
	SysTick->CTRL = 0x00000004;				//关闭定时器
}

void Delay_ms(uint32_t xms)
{
	while(xms--)
	{
		Delay_us(1000);
	}
}
 
void Delay_s(uint32_t xs)
{
	while(xs--)
	{
		Delay_ms(1000);
	}
}