#include <hal_Serial_port.hpp>

namespace HAL::UART
{
    //ȫ�ֺ���ʵ��
    IUartBus &get_uart_bus_instance()
    {
        return UartBus::instance();
    }
}