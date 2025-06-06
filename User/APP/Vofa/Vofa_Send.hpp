#pragma once

#include "../../HAL/UART/interface/uart_device.hpp"
#include "Algorithm/pid.hpp"
#include "BSP/Motor/Dji/DjiMotor.hpp"
#include <cstdint>
#include <vector>

class VofaMotorController
{
  public:
    VofaMotorController(HAL::UART::IUartDevice *uart, BSP::Motor::Dji::DjiMotorBase<1> *motor, PID &pid)
        : uart_dev(uart), motor(motor), pid_ctrl(pid), target_speed(0), enable_flag(0), time_axis(0),
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
        float freq = feedback.velocity_Rpm * 0.15f;
        // CAN控制
        float pid_output = 0;
        if (enable_flag == 0x01)
        {
            pid_output = pid_ctrl.compute(target_speed, feedback.velocity_Rpm);

            if (feedback.velocity_Rpm < -10)
            {
                time_axis += dt;

                // 判断是否超过设定的定时关闭时间
                if (time_axis >= time_stop)
                {
                    enable_flag = 0x00;
                    pid_output = 0;
                    pid_ctrl.reset();
                    motor->setCAN(0, 4);
                    motor->sendCAN();
                    
                }
            }
        }
        else
        {
            pid_output = 0;
        }

        send(feedback.angle_Deg, feedback.velocity_Rpm, time_axis, freq, 0, 0);
        motor->setCAN((int16_t)pid_output, 4);
        motor->sendCAN();
    }
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
    void setRunDuration(float seconds)
    {
        time_stop = seconds;
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

        HAL::UART::Data data{send_buf, 28};
        uart_dev->transmit_dma(data);
    }

    HAL::UART::IUartDevice *uart_dev;
    BSP::Motor::Dji::DjiMotorBase<1> *motor;
    PID &pid_ctrl;
    uint8_t send_buf[32];
    float target_speed;
    uint8_t enable_flag;
    float time_axis;
    uint32_t last_tick;
    float time_stop = 5.0f;
    int time_start = 0;
};