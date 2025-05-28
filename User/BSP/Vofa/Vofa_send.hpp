#pragma once
#include "../../APP/variables.hpp"
#include "../Motor/Dji/DjiMotor.hpp"
#include "stm32f4xx_hal.h"
#include "usart.h"
#include <string.h>

class VofaMotorController
{
  public:
    VofaMotorController(UART_HandleTypeDef *uart, BSP::Motor::Dji::DjiMotorBase<1> *motor, PID &pid)
        : huart(uart), motor(motor), pid_ctrl(pid), target_speed(0), enable_flag(0), time_axis(0),
          last_tick(HAL_GetTick())
    {
        memset(send_buf, 0, sizeof(send_buf));
    }

    void process()
    {
        uint32_t now_tick = HAL_GetTick();
        float dt = (now_tick - last_tick) / 1000.0f;
        last_tick = now_tick;

        const auto &feedback = motor->GetUnitData(0);
        if (enable_flag == 0x01)
            time_axis += dt;
        else
            time_axis = 0.0f;
        float freq = (feedback.velocity_Rpm / 60.0f) / 36.0f * 9.0f;
        send(feedback.angle_Deg, feedback.velocity_Rpm, time_axis, freq, 0, 0);

        if (enable_flag == 0x01)
        {
            float pid_output = pid_ctrl.compute(target_speed, feedback.velocity_Rpm);
            motor->setCAN((int16_t)pid_output, 4);
            motor->sendCAN(&hcan1, 0);
        }
        else
        {
            motor->setCAN(0, 4);
            motor->sendCAN(&hcan1, 0);
        }
    }

    // 可扩展接口
    float getTargetSpeed() const
    {
        return target_speed;
    }
    uint8_t getEnableFlag() const
    {
        return enable_flag;
    }
    void setTargetSpeed(float speed)
    {
        target_speed = speed;
    }
    void setEnableFlag(uint8_t flag)
    {
        enable_flag = flag;
    }

  private:
    void send(float x1, float x2, float x3, float x4, float x5, float x6)
    {
        *((float *)&send_buf[0]) = x1;
        *((float *)&send_buf[4]) = x2;
        *((float *)&send_buf[8]) = x3;
        *((float *)&send_buf[12]) = x4;
        *((float *)&send_buf[16]) = x5;
        *((float *)&send_buf[20]) = x6;
        *((uint32_t *)&send_buf[24]) = 0x7f800000;
        HAL_UART_Transmit_DMA(huart, send_buf, 28);
    }

    void recv()
    {
        uint8_t rx_buf[8];
        if (HAL_UART_Receive(huart, rx_buf, 5, 10) == HAL_OK)
        {
            if (rx_buf[0] == 0xA1)
                memcpy(&target_speed, &rx_buf[1], 4);
            else if (rx_buf[0] == 0xA2)
                enable_flag = rx_buf[1];
        }
    }

    UART_HandleTypeDef *huart;
    BSP::Motor::Dji::DjiMotorBase<1> *motor;
    PID &pid_ctrl;
    uint8_t send_buf[32];
    float target_speed;
    uint8_t enable_flag;
    float time_axis;
    uint32_t last_tick;
};
