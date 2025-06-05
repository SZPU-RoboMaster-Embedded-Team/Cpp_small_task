#include <hal_Serial_port.hpp>

namespace HAL::UART
{
    //全局函数实现
    IUartBus &get_uart_bus_instance()
    {
        return UartBus::instance();
    }
}