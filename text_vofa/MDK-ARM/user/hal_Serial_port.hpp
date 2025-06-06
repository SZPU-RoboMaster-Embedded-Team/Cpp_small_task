#include <math.h>
#include <usart.h>

/********************* INTERFACE_UART_DEVICE *********************/
namespace HAL::UART
{
    // UART���ݽṹ��
    struct Data
    {
        uint8_t *buffer; // ���ݻ�����ָ��
        uint16_t size;   // ���ݴ�С
    };

    // UART�豸����ӿ�
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

            //�������ݣ���������
            virtual bool receive(Data &data) = 0;
            
            //��ȡUART�豸���
            virtual UART_HandleTypeDef *get_handle() const = 0;

            //���͵����ֽ�
            virtual bool transmit_byte(uint8_t byte) = 0;

            //���յ����ֽ�
            virtual bool receive_byte(uint8_t &byte) = 0;

            //ʹ��DMA��������
            virtual bool transmit_dma(const Data &data) = 0;

            // ʹ��DMA��������
            virtual bool receive_dma(Data &data) = 0;

            //����DMA�������ղ�ʹ�ÿ����жϼ��
            virtual bool receive_dma_idle(Data &data) = 0;

            //���ORE���󣨴������������������DMA����
            virtual void clear_ore_error(Data &data) = 0;
    };

/********************* INTERFACE_UART_BUS *********************/
// UART�豸IDö��
enum class UartDeviceId : uint8_t
{
    HAL_UART1 = 0,
    HAL_UART2 = 1,
    HAL_UART3 = 2,
    HAL_UART4 = 3,
    HAL_UART5 = 4,
    HAL_UART6 = 5,
    HAL_UART7 = 6,
    HAL_UART8 = 7,
    MAX_DEVICES
};

//uart���߳���ӿ�
class IUartBus
{
    public:
        virtual ~IUartBus() = default;

        // ��ȡָ��UART�豸
        virtual IUartDevice *get_device(UartDeviceId id) = 0;

        //���ݾ�API��Ӧ�ó����̽ӿڣ��ı�ݷ���
        // IUartDevice &get_uart1()
        // {
        //     return get_device(UartDeviceId::HAL_UART1);
        // }
        // IUartDevice &get_uart2()
        // {
        //     return get_device(UartDeviceId::HAL_UART2);
        // }
        // IUartDevice &get_uart3()
        // {
        //     return get_device(UartDeviceId::HAL_UART3);
        // }
        // IUartDevice &get_uart4()
        // {
        //     return get_device(UartDeviceId::HAL_UART4);
        // }
        // IUartDevice &get_uart5()
        // {
        //     return get_device(UartDeviceId::HAL_UART5);
        // }
        // IUartDevice &get_uart6()
        // {
        //     return get_device(UartDeviceId::HAL_UART6);
        // }
        // IUartDevice &get_uart7()
        // {
        //     return get_device(UartDeviceId::HAL_UART7);
        // }
        // IUartDevice &get_uart8()
        // {
        //     return get_device(UartDeviceId::HAL_UART8);
        // }        

        // ���ָ��UART�豸�Ƿ����
        virtual bool has_device(UartDeviceId id) const = 0;
};

//��ȡUART���ߵ���ʵ��
IUartBus &get_uart_bus_instance();

/********************* IMPL_UART_DEVICE *********************/

//UARTӲ���豸ʵ����
class UartDevice : public IUartDevice
{
    public:
        //���캯������ʼ��UART�豸
        explicit UartDevice(const UartDevice &) = delete;

        //��������
        ~UartDevice() override = default;

        //��ֹ�����ṹ�͸�ֵ����
        UartDevice(const UartDevice &) = delete;
        UartDevice &operator=(const UartDevice &) = delete;

        //ʵ��IUartDevice�ӿ�
        void init() override;
        void start() override;
        bool transmit(const Data &data) override;
        bool receive(Data &data) override;
        bool transmit_byte(uint8_t byte) override;
        bool receive_byte(uint8_t &byte) override;
        bool transmit_dma(const Data &data) override;
        bool receive_dma(Data &data) override;

        UART_HandleTypeDef *get_handle() const override;

    private:
        UART_HandleTypeDef *handle_;
        //����״̬��־
        bool is_receiving_;
        //DMA����״̬
        bool is_dma_tx_ongoing_;        
        bool is_dma_rx_ongoing_;
        //�����ж�״̬
        bool is_idle_enabled_;
};

//UART���߹���ʵ����
class UartBus : public IUartBus
{
    public:
        //��ȡ����ʵ��
        static UartBus &instance();

        //��������
        ~UartBus() override = default;

        //ʵ��IUartBus�ӿ�
        IUartDevice &get_device(UartDeviceId id) override;
        bool has_device(UartDeviceId id) const override;

        //��ʼ��UART����
        void init();

    private:
        //˽�й��캯��������ģʽ��
        UartBus();

        //ע��һ��UART�豸
        void register_device(UartDeviceId id, UartDevice *device);

        //�Ƿ��ѳ�ʼ����־
        bool initalized_ = false;

        //��ֹ��������͸�ֵ����
        UartBus(const UartBus &) = delete;
        UartBus &operator = (const UartBus &) = delete;

        //ʹ��ָ���������̶���Ա����
        UartDevice *devices_[(size_t)UartDeviceId::MAX_DEVICES] = {nullptr};

        //ʵ���豸ʵ��
        UartDevice uart6_;
};
}


