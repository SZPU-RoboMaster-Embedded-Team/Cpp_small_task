#pragma once
#include "../interface/uart_device.hpp"

namespace HAL::UART
{

// UART硬件设备实现类
class UartDevice : public IUartDevice
{
  public:
    // 构造函数，初始化UART设备
    explicit UartDevice(UART_HandleTypeDef *handle);

    // 析构函数
    ~UartDevice() override = default;

    // 禁止拷贝构造和赋值操作
    UartDevice(const UartDevice &) = delete;
    UartDevice &operator=(const UartDevice &) = delete;

    // 实现IUartDevice接口
    void init() override;
    void start() override;
    bool transmit(const Data &data) override;
    bool receive(Data &data) override;
    bool transmit_byte(uint8_t byte) override;
    bool receive_byte(uint8_t &byte) override;
    bool transmit_dma(const Data &data) override;
    bool receive_dma(Data &data) override;
    bool receive_dma_idle(Data &data) override;
    void enable_idle_interrupt() override;
    void disable_idle_interrupt() override;
    bool is_idle_interrupt() override;
    void clear_idle_flag() override;
    void abort_dma() override;
    UART_HandleTypeDef *get_handle() const override;

  private:
    UART_HandleTypeDef *handle_;
    // 接收状态标志
    bool is_receiving_;
    // DMA传输状态
    bool is_dma_tx_ongoing_;
    bool is_dma_rx_ongoing_;
    // 空闲中断状态
    bool is_idle_enabled_;
};

} // namespace HAL::UART