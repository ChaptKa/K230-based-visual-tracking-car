#include "stm32f10x.h"
#include "project_config.h"
#include "vision.h"
#include <stdio.h>
#include <string.h>

#define VISION_BUFFER_SIZE 32
static uint8_t vision_rx_buffer[VISION_BUFFER_SIZE];
static uint8_t vision_rx_index = 0;
static uint8_t frame_started = 0;

void Vision_Init(void) {
    vision_data.deviation = 0;
    vision_data.mode = 0;
    vision_data.data_valid = 0;
    vision_rx_index = 0;
    frame_started = 0;
    
    // 初始化串口发送功能
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 启用USART1和GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
    
    // 配置TX引脚 (PA9) - 发送给K210
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  // 复用推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 配置RX引脚 (PA10) - 从K210接收
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  // 浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 配置USART1参数
    USART_InitStructure.USART_BaudRate = 115200;  // 与K210匹配的波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    
    USART_Init(USART1, &USART_InitStructure);
    
    // 使能USART1接收中断
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    
    // 配置NVIC
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 启动USART1
    USART_Cmd(USART1, ENABLE);
    
    printf("Vision module initialized with UART TX/RX\r\n");
}

// 向K210发送启动信号
void Send_StartSignal(void) {
    char start_cmd[] = "START\r\n";  // 启动信号
    printf("Sending start signal to K210: %s", start_cmd);
    
    for(int i = 0; i < strlen(start_cmd); i++) {
        USART_SendData(USART1, (uint8_t)start_cmd[i]);
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {
            // 等待发送完成
        }
    }
}

// 向K210发送任务状态
void Send_TaskStatus(uint8_t task_id) {
    char status_msg[32];
    sprintf(status_msg, "TASK%d\r\n", task_id);
    
    printf("Sending task status to K210: %s", status_msg);
    
    for(int i = 0; i < strlen(status_msg); i++) {
        USART_SendData(USART1, (uint8_t)status_msg[i]);
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {
            // 等待发送完成
        }
    }
}

// 向K210发送完成信号
void Send_FinishSignal(void) {
    char finish_cmd[] = "FINISH\r\n";
    printf("Sending finish signal to K210: %s", finish_cmd);
    
    for(int i = 0; i < strlen(finish_cmd); i++) {
        USART_SendData(USART1, (uint8_t)finish_cmd[i]);
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {
            // 等待发送完成
        }
    }
}

// 向K210发送自定义命令
void Send_CustomCommand(const char* command) {
    printf("Sending custom command to K210: %s\r\n", command);
    
    for(int i = 0; i < strlen(command); i++) {
        USART_SendData(USART1, (uint8_t)command[i]);
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {
            // 等待发送完成
        }
    }
    
    // 发送换行符
    USART_SendData(USART1, '\r');
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, '\n');
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void ParseVisionData(uint8_t* raw_data) {
    char buffer[32];
    int deviation, mode;
    
    strncpy(buffer, (char*)raw_data, sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    
    if (sscanf(buffer, "#%d,%d!", &deviation, &mode) == 2) {
        vision_data.deviation = deviation;
        vision_data.mode = mode;
        vision_data.data_valid = 1;
        
        printf("Vision data parsed: deviation=%d, mode=%d\r\n", deviation, mode);
    } else {
        vision_data.data_valid = 0;
        printf("Vision data parse failed: %s\r\n", buffer);
    }
}

void USART1_IRQHandler(void) {
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        uint8_t received_char = USART_ReceiveData(USART1);
        
        if(received_char == '#') {
            vision_rx_index = 0;
            frame_started = 1;
            vision_rx_buffer[vision_rx_index++] = received_char;
        }
        else if(received_char == '!' && frame_started) {
            if(vision_rx_index < VISION_BUFFER_SIZE - 1) {
                vision_rx_buffer[vision_rx_index++] = received_char;
                vision_rx_buffer[vision_rx_index] = '\0';
                ParseVisionData(vision_rx_buffer);
            }
            frame_started = 0;
            vision_rx_index = 0;
        }
        else if(frame_started && vision_rx_index < VISION_BUFFER_SIZE - 1) {
            vision_rx_buffer[vision_rx_index++] = received_char;
        }
        else if(vision_rx_index >= VISION_BUFFER_SIZE - 1) {
            frame_started = 0;
            vision_rx_index = 0;
            printf("Vision buffer overflow!\r\n");
        }
        
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

VisionData_t GetVisionData(void) {
    return vision_data;
}

// 测试串口发送功能
void Test_UART_Communication(void) {
    printf("Testing UART communication with K210...\r\n");
    
    // 发送测试命令
    Send_CustomCommand("TEST");
    printf("Test command sent, waiting for response...\r\n");
}