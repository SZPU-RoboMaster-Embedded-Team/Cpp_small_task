#include <hal_Serial_port.hpp>

/********************* INTERFACE_UART_BUS *********************/
namespace HAL::UART
{
    //全局函数实现
    IUartBus &get_uart_bus_instance()
    {
        return UartBus::instance();
    }

/********************* IMPL_UART_DEVICE *********************/
//UartDevice实现
//构造函数 
UartDevice::UartDevice(UART_HandleTypeDef *handle) :  handle_(handle), is_receiving_(false), is_dma_tx_ongoing_(false), is_dma_rx_ongoing_(false), is_idle_enabled_(false)
{
}

void UartDevice::init()
{
    //UART在HAL_UART_Init中初始化，无需再次初始化
}

void UartDevice::start()
{
    //启动UART中断
    __HAL_UART_ENABLE_IT(handle_, UART_IT_RXNE);
}

bool UartDevice::transmit(const Data &data)
{
    if(data.buffer == nullptr || data.size == 0)    //nullptr是空指针
    {
        return false;
    }

    HAL_StatusTypeDef status = HAL_UART_Transmit_IT(handle_, data.buffer, data.size);
    return (status == HAL_OK);
}

bool UartDevice::receive(Data &dataq)
{
    if(data.buffer == nullptr || data.size == 0)
    {
        return false;
    }

    HAL_StatusTypeDef status = HAL_UART_Receive_IT(handle_, data.buffer, data.size);

    if(status == HAL_OK)
    {
        return true;
    }
    return false;
}

bool UartDevice::transmit_byte(uint8_t byte)
{
    HAL_StatusTypeDef status = HAL_UART_Transmit_IT(handle_, &byte, 100);
    return (status == HAL_OK);
}

bool UartDevice::receive_byte(uint8_t &byte)
{
    HAL_StatusTypeDef status = HAL_UART_Receive_IT(handle_, &byte, 100);
    return (status == HAL_OK);
}

bool UartDevice::transmit_dma(const Data &data)
{
    if(data.buffer == nullptr || data.size == 0)
    {
        return false;
    }

    HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(handle_, data.buffer, data.size);
    if(status == HAL_OK)
    {
        return true;
    }
    return false;
}

bool UartDevice::receive_dma(Data &data)
{
    if(data.buffer == nullptr || data.size == 0)
    {
        return false;
    }

    HAL_StatusTypeDef status = HAL_UART_Receive_DMA(handle_, data.buffer, data.size);
    if(status == HAL_OK)
    {
        return true;
    }
    return false;
}

bool UartDevice::receive_dma_idle(Data &data)
{
    if(data.buffer == nullptr || data.size == 0)
    {
        return false;
    }

    HAL_StatusTypeDef status = HAL_UART_Receive_DMA(handle_, data.buffer, data.size);
    if(status == HAL_OK)
    {
        return true;
    }
    return false;
}

void UartDevice::clear_ore_error(Data &data)
{
    if(__HAL_UART_GET_FLAG(handle_, UART_FLAG_ORE)) //检测ORE（串口溢出）错误
    {
        __HAL_UART_CLEAR_OREFLAG(handle_);
        HAL_UARTEx_ReceiveToIdle_DMA(handle_, data.buffer, data.size);
    }
}

UART_HandleTypeDef *UartDevice::get_handle() const
{
    return handle_;
}

/********************* IMPL_UART_BUS *********************/
UartBus &UartBus::instance()
{
    static UartBus instance; //实例化UartBus类的静态成员变量
    //在第一次获取实例时初始化
    if(!instance.initialized_)
    {
        instance.init();
        instance.initialized_ = true;
    }
    return instance;
}

UartBus::UartBus() : uart6_(&huart6)
{
    //注册现有的设备
    register_device(UartDeviceId::HAL_UART6, &uart6_);
}

void UartBus::init()
{
    //初始化所有已注册的设备
    for(size_t i = 0; i < (size_t)UartDeviceId::MAX_DEVICES; ++i)
    {
        if(devices_[i] != nullptr)
        {
            devices_[i]->init();
            devices_[i]->start();
        }
    }
}

void UartBus::register_device(UartDeviceID id, UartDevice *device)
{
    if(id < UartDeviceId::MAX_DEVICES && device != nullptr)
    {
        devices_[(size_t)id] = device;
    }
}

IUartDevice &UartBus::get_device(UartDeviceId id)
{
    if(id < UartDeviceId::MAX_DEVICES && devices_[(size_t)id] != nullptr)
    {
        return *devices_[(size_t)id];
    }
    return uart6_; //如果没有可用设备，返回uart1_（保证永远有返回值）
}

bool UartBus::has_device(UartDeviceId id) const
{
    return id < UartDeviceId::MAX_DEVICES && devices_[(size_t)id] != nullptr;
}

}


