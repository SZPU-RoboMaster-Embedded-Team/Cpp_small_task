#include "uart_bus.hpp"
#include "User/BSP/UART_BSP/uart_bus_impl.hpp"

namespace HAL::UART
{

// 全局函数实现
IUartBus &get_uart_bus_instance()
{
    return UartBus::instance();
}

} // namespace HAL::UART