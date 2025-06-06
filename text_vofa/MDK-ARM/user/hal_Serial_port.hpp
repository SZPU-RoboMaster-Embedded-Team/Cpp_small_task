#include <math.h>
#include <usart.h>

/********************* INTERFACE_UART_DEVICE *********************/
namespace HAL::UART
{
    // UART数据结构体
    struct Data
    {
        uint8_t *buffer; // 数据缓冲区指针
        uint16_t size;   // 数据大小
    };

    // UART设备抽象接口
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

            //接收数据（非阻塞）
            virtual bool receive(Data &data) = 0;
            
            //获取UART设备句柄
            virtual UART_HandleTypeDef *get_handle() const = 0;

            //发送单个字节
            virtual bool transmit_byte(uint8_t byte) = 0;

            //接收单个字节
            virtual bool receive_byte(uint8_t &byte) = 0;

            //使用DMA发送数据
            virtual bool transmit_dma(const Data &data) = 0;

            // 使用DMA接收数据
            virtual bool receive_dma(Data &data) = 0;

            //设置DMA连续接收并使用空闲中断检测
            virtual bool receive_dma_idle(Data &data) = 0;

            //清楚ORE错误（串口溢出）并重新启动DMA接收
            virtual void clear_ore_error(Data &data) = 0;
    };

/********************* INTERFACE_UART_BUS *********************/
// UART设备ID枚举
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

//uart总线抽象接口
class IUartBus
{
    public:
        virtual ~IUartBus() = default;

        // 获取指定UART设备
        virtual IUartDevice *get_device(UartDeviceId id) = 0;

        //兼容旧API（应用程序编程接口）的便捷方法
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

        // 检查指定UART设备是否存在
        virtual bool has_device(UartDeviceId id) const = 0;
};

//获取UART总线单例实例
IUartBus &get_uart_bus_instance();

/********************* IMPL_UART_DEVICE *********************/

//UART硬件设备实现类
class UartDevice : public IUartDevice
{
    public:
        //构造函数，初始化UART设备
        explicit UartDevice(const UartDevice &) = delete;

        //析构函数
        ~UartDevice() override = default;

        //禁止拷贝结构和赋值操作
        UartDevice(const UartDevice &) = delete;
        UartDevice &operator=(const UartDevice &) = delete;

        //实现IUartDevice接口
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
        //接收状态标志
        bool is_receiving_;
        //DMA传输状态
        bool is_dma_tx_ongoing_;        
        bool is_dma_rx_ongoing_;
        //空闲中断状态
        bool is_idle_enabled_;
};

//UART总线管理实现类
class UartBus : public IUartBus
{
    public:
        //获取单例实例
        static UartBus &instance();

        //析构函数
        ~UartBus() override = default;

        //实现IUartBus接口
        IUartDevice &get_device(UartDeviceId id) override;
        bool has_device(UartDeviceId id) const override;

        //初始化UART总线
        void init();

    private:
        //私有构造函数（单例模式）
        UartBus();

        //注册一个UART设备
        void register_device(UartDeviceId id, UartDevice *device);

        //是否已初始化标志
        bool initalized_ = false;

        //禁止拷贝构造和赋值操作
        UartBus(const UartBus &) = delete;
        UartBus &operator = (const UartBus &) = delete;

        //使用指针数组代替固定成员变量
        UartDevice *devices_[(size_t)UartDeviceId::MAX_DEVICES] = {nullptr};

        //实际设备实例
        UartDevice uart6_;
};
}


