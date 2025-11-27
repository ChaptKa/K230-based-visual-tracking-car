/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  *
  * 串口功能全部移入串口中断执行：
  *  - 接收 1 字节
  *  - 写入 ringBuffer
  *  - 在中断内自动组帧
  *  - 解析成功的数据写入 recvFrame[]
  *  - 设置 recvReady = 1
  *
  * 主循环不再参与串口解析，仅执行小车动作。
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include "OLED.h"
#include "PWM.h"
#include "Motor.h"
#include <string.h>  // memcpy
/* USER CODE END Includes */

/* USER CODE BEGIN PV */

// 单字节缓存(中断接收)
uint8_t rxData = 0;
uint8_t Q=1;
// 环形缓冲区
#define RING_BUFFER_SIZE 128
uint8_t ringBuffer[RING_BUFFER_SIZE];

volatile uint16_t ringWriteIndex = 0;
volatile uint16_t ringReadIndex  = 0;

// 帧头尾
#define FRAME_HEAD 0xAA
#define FRAME_TAIL 0x55

// 最终解析出的有效帧
uint8_t recvFrame[64];   // 扩大到 64 字节以兼容更长 payload
uint8_t recvReady = 0;
uint8_t flash= 0;

 /* USER CODE END PV */


/* USER CODE BEGIN 0 */

/**
 * @brief 从环形缓冲区读 1 字节（中断和主循环都可以调用）
 * @return 1 有数据并通过 *byte 返回，0 无数据
 */
static inline int RingBuffer_ReadByte(uint8_t *byte)
{
    if (ringReadIndex == ringWriteIndex)
        return 0;

    *byte = ringBuffer[ringReadIndex];
    ringReadIndex = (ringReadIndex + 1) % RING_BUFFER_SIZE;
    return 1;
}

/**
 * @brief 串口数据组帧（本函数现在在 UART 中断中调用）
 *
 * 帧格式：
 *   0xAA LEN_L LEN_H PAYLOAD... 0x55
 *
 * 功能：
 *   - 从环形缓冲区读取字节并组帧
 *   - 若组出完整并校验通过的帧，则拷贝到 recvFrame 并设置 recvReady=1
 *
 * 注意：此函数可能在中断上下文被调用（你的设计里是这样的），
 *       因此函数内部尽量避免耗时操作（但拷贝小数组和设置标志是允许的）。
 */
void UART_ProcessFrame_InIRQ(void)
{
    static uint8_t frameBuf[128];   // 暂存正在拼接的帧
    static uint16_t frameIndex = 0;
    uint8_t data;

    // 从环形缓冲区拉数据直到用完
    while (RingBuffer_ReadByte(&data))
    {
        frameBuf[frameIndex++] = data;

        /* 帧头检查：第一个字节必须为 FRAME_HEAD，否则丢弃重来 */
        if (frameIndex == 1 && frameBuf[0] != FRAME_HEAD)
        {
            frameIndex = 0;
            continue;
        }

        /* 当收到 >=3 字节可以解析长度字段 */
        if (frameIndex >= 3)
        {
            uint16_t length = frameBuf[1] | (frameBuf[2] << 8);   // 小端长度
            uint16_t need_len = length + 4; // head(1)+len(2)+payload+tail(1)

            /* 收到完整帧 */
            if (frameIndex >= need_len)
            {
                /* 校验帧尾 */
                if (frameBuf[need_len - 1] == FRAME_TAIL)
                {
                    /* 安全拷贝整帧至 recvFrame（主循环可读取） */
                    // 注意：recvFrame 大小为 64，确保 need_len <= sizeof(recvFrame)
                    if (need_len <= sizeof(recvFrame))
                    {
                        memcpy(recvFrame, frameBuf, need_len);
                        // 标记接收完成，主循环会处理 recvFrame 的内容
                        recvReady = 1;
                    }
                    else
                    {
                        // 帧太长，丢弃（或者可做其他处理）
                    }
                }
                /* 不论是否校验成功，都重置拼帧状态以接收下一帧 */
                frameIndex = 0;
            }

            /* 保险：防止 frameBuf 溢出 */
            if (frameIndex >= sizeof(frameBuf))
                frameIndex = 0;
        }
    }
}

/**
 * @brief UART 中断回调
 *
 * 功能：
 *   1. 将接收到的字节写入 ringBuffer
 *   2. 再次启动接收（IT 模式）
 *   3. 在中断内尝试组帧并设置 recvReady（你的设计要求在中断内完成串口解析）
 *
 * 注意：
 *   - 该回调要尽量短小；你现在在中断内做了组帧（相对轻量），这是可以的，
 *     但如果帧量大或帧处理复杂，建议把组帧放主循环以减少中断占用时间。
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        // 将单字节写入环形缓冲区
        ringBuffer[ringWriteIndex] = rxData;
        ringWriteIndex = (ringWriteIndex + 1) % RING_BUFFER_SIZE;

        // 如果写满，移动读指针以覆盖最旧数据（保持环形缓冲区工作）
        if (ringWriteIndex == ringReadIndex)
            ringReadIndex = (ringReadIndex + 1) % RING_BUFFER_SIZE;

        // 立即再次开启下一字节中断接收（必须在最后或接近最后）
        HAL_UART_Receive_IT(&huart2, &rxData, 1);

        // 在中断内尽量轻量地解析帧，这里尝试把环形缓存里的数据组帧
        UART_ProcessFrame_InIRQ();
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  HAL_Init();

  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();

  /* PWM 开启 */
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);

  /* 板载灯 初始状态 */

  //OLED_Init();

  /* 启动串口中断接收（一次1字节） */
  HAL_UART_Receive_IT(&huart2, &rxData, 1);

  //OLED_ShowString(1, 1, "wait");
  uint8_t flag =0 ;
  uint8_t flag_1 = 0;
  uint16_t flag_2 =0;
  uint16_t break1 =0;
  uint16_t break3 =0;
  /* ----------- 主循环 ----------- */
  while (1)
  {
        // 保持板载指示GPIO（你原有的行为）
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_14,GPIO_PIN_SET);

        /* 有新的完整解析帧可处理（recvFrame 中包含整帧） */
	  

        if (recvReady)
        {
            recvReady = 0;  // 先清标志，开始处理（确保不会重复处理）

            // 解析长度与 payload（安全访问）
            uint16_t length = recvFrame[1] | (recvFrame[2] << 8);
            // payload 首地址位于 recvFrame[3]，长度为 length

            // 兼容性处理：当 length < 2 时，视为无效帧
            if (length < 2)
            {
                // 无效帧：忽略
                continue;
            }

            // 取最少的两个值（type / adjust）
            uint8_t type   = recvFrame[3];   // payload[0]
            uint8_t adjust = recvFrame[4];   // payload[1]

            // 这里做兼容：如果视觉端发送了第三个字节（up_code），就读取它；否则设为0
            uint8_t up_code = 0x00;
            if (length >= 3)
            {
                up_code = recvFrame[5 - 2 + 2]; // 解释：payload start at index3 => payload[2] at index5
                // 简化上面索引（更直观写法）：
                up_code = recvFrame[3 + 2]; // payload[2]
            }

			if(Q == 1)
			{
				
			if(flag==0)
			{
				if(type ==  0x03 || type==0x05 || type==0x06 || type== 0x07 )
				{
					flag=1;
					//OLED_ShowNum(1, 1, Q, 1 );
				}
			switch (type)
			{
				case 0x00 : switch (adjust) {case 0x00:car_go(); break; case 0x01:car_turnleft_s(); break; case 0x02:car_turnright_s(); break;
											 case 0x03:car_turnright_save(); break; case 0x04:car_turnleft_save(); break;} break;
				case 0x01 : switch (adjust) {case 0x00:car_go(); break; case 0x01:car_turnleft_s(); break; case 0x02:car_turnright_s(); break;
											 case 0x03:car_turnright_save(); break; case 0x04:car_turnleft_save(); break;}break;
				case 0x02 : switch (adjust) {case 0x00:car_go(); break; case 0x01:car_turnleft_s(); break; case 0x02:car_turnright_s(); break;
											 case 0x03:car_turnright_save(); break; case 0x04:car_turnleft_save(); break;} break;
				case 0x03 : car_stop();HAL_Delay(300);car_turnright_b();HAL_Delay(500); break;
				case 0x04 : switch (adjust) {case 0x00:car_go(); break; case 0x01:car_turnleft_s(); break; case 0x02:car_turnright_s(); break;
											 case 0x03:car_turnright_save(); break; case 0x04:car_turnleft_save(); break;}break;
				case 0x05 : car_stop();HAL_Delay(300);car_turnright_b();HAL_Delay(500);Q++;HAL_GPIO_WritePin(GPIOC,GPIO_PIN_14,GPIO_PIN_SET); break;
				case 0x06 : car_stop();HAL_Delay(300);car_turnright_b();HAL_Delay(500);break;	
				case 0x07 : car_stop();HAL_Delay(300);car_turnright_b();HAL_Delay(500);break;				
			}
			}
			else if(flag == 1 && up_code == 0 )
			{	
				//OLED_ShowNum(1, 1, Q, 1 );
				car_turnright_b();
			}
			else if(flag == 1 && up_code == 1)
			{
				flag = 0;
				ringReadIndex = ringWriteIndex - 1;
			}
			}
			if(Q ==2)
			{
//				HAL_GPIO_WritePin(GPIOC,GPIO_PIN_14,GPIO_PIN_SET);
				if(flag == 0)
				{
					if(type == 0x05)
					{
						flag=1;
						//OLED_ShowNum(1, 1, Q, 1 );
					}
								switch (type)
			{
				case 0x00 : switch (adjust) {case 0x00:car_go_2(); break; case 0x01:car_turnleft_s_2(); break; case 0x02:car_turnright_s_2(); break;
											 case 0x03:car_turnright_save(); break; case 0x04:car_turnleft_save(); break;} break;
				case 0x01 : switch (adjust) {case 0x00:car_go_2(); break; case 0x01:car_turnleft_s_2(); break; case 0x02:car_turnright_s_2(); break;
											 case 0x03:car_turnright_save(); break; case 0x04:car_turnleft_save(); break;} break;
				case 0x02 : switch (adjust) {case 0x00:car_go_2(); break; case 0x01:car_turnleft_s_2(); break; case 0x02:car_turnright_s_2(); break;
											 case 0x03:car_go_2();break; case 0x04:car_go_2(); break;} break;	
				case 0x03 : switch (adjust) {case 0x00:car_go_2(); break; case 0x01:car_turnleft_s_2(); break; case 0x02:car_turnright_s_2(); break;
											 case 0x03:car_go_2();break; case 0x04:car_go_2(); break;} break;	
				case 0x04 : switch (adjust) {case 0x00:car_go_2(); break; case 0x01:car_turnleft_s_2(); break; case 0x02:car_turnright_s_2(); break;
											 case 0x03:car_go_2();break; case 0x04:car_go_2(); break;} break;	
				case 0x05 : car_stop();HAL_Delay(300);car_turnright_b();HAL_Delay(500);flag_1++;if(flag_1==4){Q++;}break;
				case 0x06 : car_go_2();break;
					//switch (adjust) {case 0x00:car_go_2(); break; case 0x01:car_turnleft_s_2(); break; case 0x02:car_turnright_s_2(); break;
											 //case 0x03:car_go_2();break; case 0x04:car_go_2();break;} break;
				case 0x07 : switch (adjust) {case 0x00:car_go_2(); break; case 0x01:car_turnleft_s_2(); break; case 0x02:car_turnright_s_2(); break;
											 case 0x03:car_go_2();break; case 0x04:car_go_2(); break;} break;			
			}	
			
			}
			else if(flag == 1 && up_code == 0 )
			{	
				//OLED_ShowNum(1, 1, Q, 1 );
				car_turnright_b();
			}
			else if(flag == 1 && up_code == 1)
			{
				flag = 0;
			}
			}
		if(Q==3)
		{
			flash=1;
			if(flag==0)
			{
				if(type == 0x06 )
				{
					flag=1;
					//OLED_ShowNum(1, 1, Q, 1 );
				}
				else if (type == 0x02)
				{
					flag = 2;
					//OLED_ShowNum(1, 1, Q, 1 );
				}
				else if (type == 0x04)
				{
					flag =2;
					//OLED_ShowNum(1, 1, Q, 1 );
				}
				else if(type == 0x07&&flag_2==0)
				{
					flag=2;
					//OLED_ShowNum(1, 1, Q, 1 );
				}
				else if(type == 0x07&&flag_2>=1)
				{
					flag=1;
					//OLED_ShowNum(1, 1, Q, 1 );
				}
					switch (type)
			{
				
				case 0x00 : switch (adjust) {case 0x00:car_go(); break; case 0x01:car_turnleft_s(); break; case 0x02:car_turnright_s(); break;
											 case 0x03:car_turnright_save(); break; case 0x04:car_turnleft_save(); break;} break;
				case 0x01 : switch (adjust) {case 0x00:car_go(); break; case 0x01:car_turnleft_s(); break; case 0x02:car_turnright_s(); break;
											 case 0x03:car_turnright_save(); break; case 0x04:car_turnleft_save(); break;} break;
				case 0x02 : if(break3==0){car_stop();HAL_Delay(300);car_turnleft_b();
				HAL_Delay(500);break3++;}else{car_go();}break;
				case 0x03 : car_go_2();switch (adjust) {case 0x00:car_go(); break; case 0x01:car_turnleft_s(); break; case 0x02:car_turnright_s(); break;
											 case 0x03:car_turnright_save(); break; case 0x04:car_turnleft_save(); break;} break;
				case 0x04 : if(flag_2<=1){car_stop();HAL_Delay(300);car_turnleft_b();
				HAL_Delay(500);flag_2++;}else{car_go();}break;
				case 0x05 : car_go();flash=0;break;
				case 0x06 : if(break1==0){car_stop();HAL_Delay(300);car_turnright_b();
				HAL_Delay(500);break1++;}else{car_go();}break;
				case 0x07 :	if(flag_2==0){car_stop();HAL_Delay(300);car_turnleft_b();HAL_Delay(500);flag_2++;}
							else{car_stop();HAL_Delay(300);car_turnright_b();
				HAL_Delay(500);} break;			
			}	
			 
			}
			else if(flag == 1 && up_code == 0 )
			{	
				//OLED_ShowNum(1, 1, Q, 1 );
				car_turnright_b();
			}
			else if(flag ==2 && up_code == 0 )
			{
				//OLED_ShowNum(1, 1, Q, 1 );
				car_turnleft_b();
			}
			else if(flag == 1 && up_code == 1)
			{
				flag = 0;
			}
			else if(flag == 2 && up_code == 1)
			{
				flag = 0;
			}
		}

            /* ==== OLED 显示：显示 type / adjust / up_code ==== */
            //OLED_Clear();
            //OLED_ShowNum(1, 1, Q, 1 );
            //OLED_ShowString(2, 1, "Type:");
            //OLED_ShowNum(2, 7, type, 2);
            //OLED_ShowString(3, 1, "Adj:");
            //OLED_ShowNum(3, 6, adjust, 2);
            //OLED_ShowString(4, 1, "Up:");
            //OLED_‘ShowSignedNum(4, 5, ringWriteIndex-ringReadIndex, 1); // up_code 为 0x00 或 0x01，显示占位

        } /* end if (recvReady) */
  } /* end while */
}


/* ---------------- 系统时钟配置（保持原样） ---------------- */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
      Error_Handler();

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
      Error_Handler();
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}
//定时器中断
// 1. 新增全局计数变量
//uint16_t flash_cnt;
// 2. 重写定时器中断回调函数（加计数分频）
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
//{
//    if (htim->Instance == TIM3 && flash == 1)
//    {
//        flash_cnt++;
//        if (flash_cnt >= 2000) // 500×1ms=500ms，每500ms翻转一次
//        {

//			HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_14);
//            flash_cnt = 0; // 重置计数
//        }
//    }
//}
