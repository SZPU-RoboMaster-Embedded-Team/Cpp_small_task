#include <math.h>
#include <usart.h>

namespace HAL::UART
{
    // UART���ݽṹ��
    struct Data
    {
        uint8_t *buffer; // ���ݻ�����ָ��
        uint16_t size;   // ���ݴ�С
    };

    class IUartDevice
    {
        public:
            virtual ~IUartDevice() = default;

            // ��ʼ��UART�豸
            virtual void init() = 0;

            //����UART�豸
            virtual void start() = 0;

            // �������ݣ���������
            virtual bool transmit(const Data &data) = 0;

    }
}