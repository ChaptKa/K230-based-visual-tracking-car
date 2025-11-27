#include "buzzer.h"
#include "Delay.h"

// 假设蜂鸣器连接到GPIOB Pin12（根据实际硬件调整）
#define BUZZER_GPIO_PORT GPIOB
#define BUZZER_GPIO_PIN  GPIO_Pin_12

void Buzzer_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 启用GPIOB时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    // 配置蜂鸣器引脚为推挽输出
    GPIO_InitStructure.GPIO_Pin = BUZZER_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BUZZER_GPIO_PORT, &GPIO_InitStructure);
    
    // 初始状态为关闭
    GPIO_ResetBits(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN);
}

void Buzzer_TimeoutAlert(void) {
    // 发出3声短促的蜂鸣声作为超时警告
    for(int i = 0; i < 3; i++) {
        GPIO_SetBits(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN);  // 蜂鸣器响
        Delay_ms(200);
        GPIO_ResetBits(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN); // 蜂鸣器停
        Delay_ms(200);
    }
}

void Buzzer_Stop(void) {
    GPIO_ResetBits(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN);
}