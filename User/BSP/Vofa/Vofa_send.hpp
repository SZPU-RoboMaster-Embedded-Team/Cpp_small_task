#pragma once
#include "stm32f4xx_hal.h"
#include "usart.h"
#include <string.h>
uint8_t send_str2[64];
float vofa_recv_buf[6];
float vofa_target_speed = 0;  // 新增：目标速度
uint8_t vofa_enable_flag = 0; // 新增：使能标志
// Vofa发送
void vofaSend(float x1, float x2, float x3, float x4, float x5, float x6)
{
    const uint8_t sendSize = 4;

    *((float *)&send_str2[sendSize * 0]) = x1;
    *((float *)&send_str2[sendSize * 1]) = x2;
    *((float *)&send_str2[sendSize * 2]) = x3;
    *((float *)&send_str2[sendSize * 3]) = x4;
    *((float *)&send_str2[sendSize * 4]) = x5;
    *((float *)&send_str2[sendSize * 5]) = x6;

    *((uint32_t *)&send_str2[sizeof(float) * (7)]) = 0x7f800000;
    HAL_UART_Transmit_DMA(&huart6, send_str2, sizeof(float) * (7 + 1));
}

// Vofa接收
void vofaRecv()
{
    uint8_t rx_buf[8];                                      // 1字节帧头+最多4字节float+3字节冗余
    if (HAL_UART_Receive(&huart6, rx_buf, 5, 10) == HAL_OK) // 5字节：帧头+4字节float
    {
        if (rx_buf[0] == 0xA1)
        {
            memcpy(&vofa_target_speed, &rx_buf[1], 4); // 速度
        }
        else if (rx_buf[0] == 0xA2)
        {
            vofa_enable_flag = rx_buf[1]; // 01开，00关
        }
    }
}
