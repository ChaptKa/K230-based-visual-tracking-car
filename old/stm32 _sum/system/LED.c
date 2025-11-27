#include "led.h"
#include "Delay.h"

// 假设LED连接到GPIOA Pin5（根据实际硬件调整）
#define LED_GPIO_PORT GPIOA
#define LED_GPIO_PIN  GPIO_Pin_5

static LED_State_t led_state = LED_OFF;
static uint32_t last_blink_time = 0;
static uint8_t led_current_state = 0;

void LED_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 启用GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    // 配置LED引脚为推挽输出
    GPIO_InitStructure.GPIO_Pin = LED_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_GPIO_PORT, &GPIO_InitStructure);
    
    // 初始状态为关闭
    GPIO_ResetBits(LED_GPIO_PORT, LED_GPIO_PIN);
    led_state = LED_OFF;  // 明确设置状态
}

void LED_SetState(LED_State_t state) {
    led_state = state;
    
    switch(led_state) {
        case LED_OFF:
            GPIO_ResetBits(LED_GPIO_PORT, LED_GPIO_PIN);
            led_current_state = 0;
            break;
        case LED_ON:
            GPIO_SetBits(LED_GPIO_PORT, LED_GPIO_PIN);
            led_current_state = 1;
            break;
        case LED_BLINK:
            // 闪烁状态由LED_Update处理
            // 初始化闪烁状态
            GPIO_SetBits(LED_GPIO_PORT, LED_GPIO_PIN);
            led_current_state = 1;
            last_blink_time = GetSystemTime();  // 假设有这个函数
            break;
    }
}

void LED_Update(void) {
    if (led_state == LED_BLINK) {
        uint32_t current_time = GetSystemTime();  // 需要确保这个函数存在
        
        // 每500ms切换一次LED状态
        if (current_time - last_blink_time > 500) {
            last_blink_time = current_time;
            led_current_state = !led_current_state;
            
            if (led_current_state) {
                GPIO_SetBits(LED_GPIO_PORT, LED_GPIO_PIN);
            } else {
                GPIO_ResetBits(LED_GPIO_PORT, LED_GPIO_PIN);
            }
        }
    }
}

// 获取当前LED状态
LED_State_t LED_GetState(void) {
    return led_state;
}