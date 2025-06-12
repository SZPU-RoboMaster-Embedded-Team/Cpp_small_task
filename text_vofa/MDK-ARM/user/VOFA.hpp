#ifndef  _VOFA_HPP_
#define  _VOFA_HPP_

#include <hal_Serial.h>

namespace UART::VOFA
{
    struct VOFAData 
    {
    uint8_t head;
    float is_open;
    float speed;
    };

    class VOFAManager {
    public:
        static void uartRxEventCallback(UART_HandleTypeDef *huart, uint16_t size) 
        {
            vofa.head = data[0];
            if (vofa.head == 0x01) {
                vofa.is_open = R4(data + 1);
            }
            if (vofa.head == 0x00) {
                vofa.speed = R4(data + 1);
            }
        }

        static float R4(uint8_t* p) 
        {
            float r;
            memcpy(&r, p, sizeof(float));
            return r;
        }

        // ȫ�ֶ��巢�ͻ�����������Ϊȫ�ֱ�����
        uint8_t send_str2[sizeof(float) * 8]; // ����8��float�ռ䣨32�ֽڣ�

        static void vofa_send(float x1, float x2, float x3, float x4, float x5, float x6) {
            const uint8_t sendSize = sizeof(float); // ��������ռ4�ֽ�

            // ��6����������д�뻺������С��ģʽ��
            *((float*)&send_str2[sendSize * 0]) = x1;
            *((float*)&send_str2[sendSize * 1]) = x2;
            *((float*)&send_str2[sendSize * 2]) = x3;
            *((float*)&send_str2[sendSize * 3]) = x4;
            *((float*)&send_str2[sendSize * 4]) = x5;
            *((float*)&send_str2[sendSize * 5]) = x6;

            // д��֡β��Э��Ҫ�� 0x00 0x00 0x80 0x7F��
            *((uint32_t*)&send_str2[sizeof(float) * 6]) = 0x7F800000; // С�˴洢Ϊ 00 00 80 7F

            // ͨ��DMA��������֡��6���� + 1֡β = 7��float����28�ֽڣ�
            HAL_UART_Transmit_DMA(&huart6, send_str2, sizeof(float) * 7);
        }
    private:
        static inline VOFAData vofa;
        static inline uint8_t data[4];
    };
}

#endif
