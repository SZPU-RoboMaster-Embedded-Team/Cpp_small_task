#include "../APP/Vofa/Vofa_Send.hpp"
#include "../APP/variables.hpp"
#include "../BSP/Motor/Dji/DjiMotor.hpp"
#include "../User/BSP/Motor/Dji/DjiMotor.hpp"
#include "../User/HAL/CAN/can_hal.hpp"
#include "../User/HAL/UART/uart_hal.hpp"
#include <cstring>
uint8_t buffer[8] = {0};
auto uatr_rx_frame = HAL::UART::Data{buffer, 7};

auto &uart6 = HAL::UART::get_uart_bus_instance().get_device(HAL::UART::UartDeviceId::HAL_Uart6);
VofaMotorController Vofa_Control(&uart6, &BSP::Motor::Dji::Motor2006, pid_vel_204);
extern "C"
{
    void Init()
    {
        HAL::CAN::get_can_bus_instance();

        auto &uart6 = HAL::UART::get_uart_bus_instance().get_device(HAL::UART::UartDeviceId::HAL_Uart6);
        uart6.receive_dma_idle(uatr_rx_frame);
        uart6.transmit(uatr_rx_frame);
    }

    void InWhile()
    {
        Vofa_Control.process();
    }
} // extern "C"

uint16_t speed = 0;
uint8_t data[8] = {0};
uint16_t target_speed = 0;
HAL::CAN::Frame rx_frame;
uint32_t pos = 0;

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    auto &can1 = HAL::CAN::get_can_bus_instance().get_device(HAL::CAN::CanDeviceId::HAL_Can1);
    can1.receive(rx_frame);
    if (hcan == can1.get_handle())
    {
        BSP::Motor::Dji::Motor2006.Parse(rx_frame);
        // target_speed = (rx_frame.data[6] << 8) | rx_frame.data[7];
        // 控制ID为4的2006电机，例如目标速度为1000
        //BSP::Motor::Dji::Motor2006.setCAN(target_speed, 4); // 4表示第4个电机
        BSP::Motor::Dji::Motor2006.sendCAN();
    }
}

// UART中断
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    auto &uart6 = HAL::UART::get_uart_bus_instance().get_device(HAL::UART::UartDeviceId::HAL_Uart6);

    if (huart == uart6.get_handle())
    {
        if (buffer[0] == 0xA2)
        {
            if (buffer[1] == 0x00)
            {
                Vofa_Control.setEnableFlag(0x00); // 失能
            }
            else if (buffer[1] == 0x01)
            {
                Vofa_Control.setEnableFlag(0x01); // 使能
            }
        }
        else if (buffer[0] == 0xA1)
        {
            uint16_t slider_value = (buffer[3] << 8) | buffer[4];
            float target_speed;
            // 计算 target_speed
            target_speed = (0.09139f * slider_value + 1511.0f) / 2.0f;
            Vofa_Control.setTargetSpeed(target_speed); // 设置目标速度
        }
        uart6.transmit(uatr_rx_frame);
    }
    uart6.receive_dma_idle(uatr_rx_frame);
}