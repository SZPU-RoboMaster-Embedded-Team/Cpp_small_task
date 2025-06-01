#include "Init.hpp"
#include "../BSP/CAN_BSP/can_bus_impl.hpp"
#include "../BSP/UART_BSP/uart_bus_impl.hpp"

bool InitFlag = false;
void Init()
{

    HAL::CAN::CanBus::instance();   // 初始化CAN BSP
    HAL::UART::UartBus::instance(); // 初始化UART BSP

    InitFlag = true;
}
