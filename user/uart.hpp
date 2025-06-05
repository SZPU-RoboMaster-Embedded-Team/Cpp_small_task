#include <math.h>
#include <usart.h>

namespace HAL::UART
{
    // UART数据结构体
    struct Data
    {
        uint8_t *buffer; // 数据缓冲区指针
        uint16_t size;   // 数据大小
    };

    class IUartDevice
    {
        public:
            virtual ~IUartDevice() = default;

            // 初始化UART设备
            virtual void init() = 0;

            //启动UART设备
            virtual void start() = 0;

            // 发送数据（非阻塞）
            virtual bool transmit(const Data &data) = 0;

    }
}