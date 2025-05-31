#include "uart_device_impl.hpp"

namespace HAL::UART
{

// UartDevice实现
UartDevice::UartDevice(UART_HandleTypeDef *handle)
    : handle_(handle), is_receiving_(false), is_dma_tx_ongoing_(false), is_dma_rx_ongoing_(false),
      is_idle_enabled_(false)
{
}

void UartDevice::init()
{
    // UART在HAL_UART_Init中已经完成了初始化
    // 如果需要其他初始化操作，可以在这里添加
}

void UartDevice::start()
{
    // 启用UART中断
    __HAL_UART_ENABLE_IT(handle_, UART_IT_RXNE);
}

bool UartDevice::transmit(const Data &data)
{
    if (data.buffer == nullptr || data.size == 0)
    {
        return false;
    }

    HAL_StatusTypeDef status = HAL_UART_Transmit_IT(handle_, data.buffer, data.size);
    return (status == HAL_OK);
}

bool UartDevice::receive(Data &data)
{
    if (data.buffer == nullptr || data.size == 0)
    {
        return false;
    }

    if (is_receiving_)
    {
        return false; // 已经在接收中
    }

    HAL_StatusTypeDef status = HAL_UART_Receive_IT(handle_, data.buffer, data.size);
    if (status == HAL_OK)
    {
        is_receiving_ = true;
        return true;
    }
    return false;
}

bool UartDevice::transmit_byte(uint8_t byte)
{
    HAL_StatusTypeDef status = HAL_UART_Transmit(handle_, &byte, 1, 100);
    return (status == HAL_OK);
}

bool UartDevice::receive_byte(uint8_t &byte)
{
    HAL_StatusTypeDef status = HAL_UART_Receive(handle_, &byte, 1, 100);
    return (status == HAL_OK);
}

bool UartDevice::transmit_dma(const Data &data)
{
    if (data.buffer == nullptr || data.size == 0)
    {
        return false;
    }

    if (is_dma_tx_ongoing_)
    {
        return false; // DMA发送已经在进行中
    }

    HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(handle_, data.buffer, data.size);
    if (status == HAL_OK)
    {
        is_dma_tx_ongoing_ = true;
        return true;
    }
    return false;
}

bool UartDevice::receive_dma(Data &data)
{
    if (data.buffer == nullptr || data.size == 0)
    {
        return false;
    }

    if (is_dma_rx_ongoing_)
    {
        return false; // DMA接收已经在进行中
    }

    HAL_StatusTypeDef status = HAL_UART_Receive_DMA(handle_, data.buffer, data.size);
    if (status == HAL_OK)
    {
        is_dma_rx_ongoing_ = true;
        return true;
    }
    return false;
}

bool UartDevice::receive_dma_idle(Data &data)
{
    // 启动DMA连续接收
    if (receive_dma(data))
    {
        // 启用空闲中断检测
        enable_idle_interrupt();
        return true;
    }
    return false;
}

void UartDevice::enable_idle_interrupt()
{
    // 启用UART的IDLE中断
    __HAL_UART_ENABLE_IT(handle_, UART_IT_IDLE);
    is_idle_enabled_ = true;
}

void UartDevice::disable_idle_interrupt()
{
    // 禁用UART的IDLE中断
    __HAL_UART_DISABLE_IT(handle_, UART_IT_IDLE);
    is_idle_enabled_ = false;
}

bool UartDevice::is_idle_interrupt()
{
    // 检查是否发生IDLE中断
    return __HAL_UART_GET_FLAG(handle_, UART_FLAG_IDLE) != RESET;
}

void UartDevice::clear_idle_flag()
{
    // 清除IDLE标志位
    __HAL_UART_CLEAR_IDLEFLAG(handle_);
}

void UartDevice::abort_dma()
{
    if (is_dma_tx_ongoing_ || is_dma_rx_ongoing_)
    {
        HAL_UART_DMAStop(handle_);
        is_dma_tx_ongoing_ = false;
        is_dma_rx_ongoing_ = false;
    }
}

UART_HandleTypeDef *UartDevice::get_handle() const
{
    return handle_;
}

} // namespace HAL::UART