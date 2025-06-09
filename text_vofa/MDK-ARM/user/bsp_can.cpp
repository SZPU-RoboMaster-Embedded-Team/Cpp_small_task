#include <hal_can.hpp>
#include <hal_Serial_port.hpp>
#include <DJI_motor.hpp>
#include <cstring>

uint8_t buffer[3] = {0};
auto uart_rx_frame = HAL::UART::Data{buffer, 3};    //结构体Data初始化，有数组和大小

extern "c"
{
    void Init()
    {
        HAL::CAN::get_can_bus_instance();

        auto &uart6 = HAL::UART::get_uart_bus_instance().get_device(HAL::UART::UartDeviceId::HAL_UART6);
        uart6->receive_dma_idle(uart_rx_frame);
        uart6->transmit(uart_rx_frame);
    }

    void InWhile()
    {
    }
}

uint16_t speed = 0;
uint8_t data[8] = {0};

HAL::CAN:: Can_data rx_frame;
uint32_t pos = 0;

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    auto &can1 = HAL::CAN::get_can_bus_instance().get_device(HAL::CAN::CanDeviceId::HAL_CAN1);
    can1.receive(rx_frame);
    if(hcan == can1.get_handle())
    {
        BSP::Motor::Motor2006.Parse(rx_frame);
        pos = static_cast<uint32_t>(BSP::Motor::Motor2006.GetPos());
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    auto &uart6 = HAL::UART::get_uart_bus_instance().get_device(HAL::UART::UartDeviceId::HAL_UART6);
    if(huart == uart6.get_handle())
    {
        // 读取接收到的数据
        uart6->read(uart_rx_frame); // 假设 read 方法可以读取接收到的数据到 uart_rx_frame

        // 解析接收到的数据（例如解析 VOFA 数据）
        UART::VOFA::VOFAManager::uartRxEventCallback(huart, Size);

        // 根据需要进行进一步处理（例如更新目标速度等）
        float target_speed = UART::VOFA::VOFAManager::vofa.speed;

        // 如果需要发送数据，准备发送数据
        // 这里假设 uart_tx_frame 是要发送的数据
        // auto uart_tx_frame = ...; // 根据需求填充 uart_tx_frame

        // 发送数据
        // uart6->transmit_dma(uart_tx_frame);
    }
}
