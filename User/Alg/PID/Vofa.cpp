#include "User/HAL/CAN/impl/can_device_impl.hpp"
#include "User/APP/CallBack/callback.cpp"
#include "User/HAL/CAN/impl/can_bus_impl.hpp"


#include "HAL/UART/uart_hal.hpp"
#include "HAL/CAN/can_hal.hpp"
#include "User/BSP/Motor/Dji/DjiMotor.hpp"

bool can_send_enable = false; // button控制CAN发送开关
float target_frequency = 0.0f; // frequency控制目标转速
#include "HAL/CAN/can_hal.hpp"



// CAN1 FIFO0接收中断回调
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    auto& can_bus = HAL::CAN::get_can_bus_instance();
    HAL::CAN::Frame rx_frame;

    // 判断是CAN1
    if (hcan == can_bus.get_can1().get_handle())
    {
        if (can_bus.get_can1().receive(rx_frame))
        {
            // 解析rx_frame，处理DM2006反馈
             BSP::Motor::Dji::Motor2006.Parse(rx_frame);
        }
    }
}

void vofa_uart_init()
{
    auto& uart1 = HAL::UART::get_uart_bus_instance().get_uart1();
    uart1.receive(uart1_rx_data);
}

// 8字节
uint8_t rx_buffer[8];
HAL::UART::Data uart1_rx_data{rx_buffer, sizeof(rx_buffer)};

// UART1接收完成回调（CubeMX生成的回调或手动调用）
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    auto& uart1 = HAL::UART::get_uart_bus_instance().get_uart1();

    if (huart == uart1.get_handle())
    {
        // 判断是否为button指令
        if (rx_buffer[0] == 0xA5)
        {
            if (rx_buffer[3] == 0x00)
                can_send_enable = false;
            else if (rx_buffer[3] == 0x01)
                can_send_enable = true;
        }
        // 判断是否为frequency指令
        else if (rx_buffer[0] == 0xA5 && rx_buffer[1] == 0x01)
        {
            float freq = 0.0f;
            memcpy(&freq, &rx_buffer[2], sizeof(float));
            target_frequency = freq;
        }

        // 继续接收下一帧
        uart1.receive(uart1_rx_data);
        uart1.clear_ore_error(uart1_rx_data);//阻止ORE
    }
}


void send_dm2006_command()
{
    if (!can_send_enable) return; // 由button控制是否允许发送

    auto& can_bus = HAL::CAN::get_can_bus_instance();
    HAL::CAN::Frame frame;
    frame.id = 0x200;
    frame.dlc = 8;
    frame.is_extended_id = false;
    frame.is_remote_frame = false;

    // 根据target_frequency生成控制数据
    int16_t speed = static_cast<int16_t>(target_frequency); // 示例：直接赋值
    frame.data[0] = (speed >> 8) & 0xFF;
    frame.data[1] = speed & 0xFF;
    frame.data[2] = (speed >> 8) & 0xFF;
    frame.data[3] = speed & 0xFF;
    frame.data[4] = (speed >> 8) & 0xFF;
    frame.data[5] = speed & 0xFF;
    frame.data[6] = (speed >> 8) & 0xFF;
    frame.data[7] = speed & 0xFF;

    //for (int i = 2; i < 8; ++i) frame.data[i] = 0;

    can_bus.get_can1().send(frame);
}

