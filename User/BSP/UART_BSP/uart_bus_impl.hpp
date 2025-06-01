#pragma once
#include "../..//HAL/UART/uart_hal.hpp"
#include "uart_device_impl.hpp"

namespace HAL::UART
{

// UART总线管理实现类
class UartBus : public IUartBus
{
  public:
    // 获取单例实例
    static UartBus &instance();

    // 析构函数
    ~UartBus() override = default;

    // 实现IUartBus接口
    IUartDevice &get_device(UartDeviceId id) override;
    bool has_device(UartDeviceId id) const override;

    // 初始化UART总线（私有，由instance()调用）
    void init();

  private:
    // 私有构造函数（单例模式）
    UartBus();

    // 注册一个UART设备
    void register_device(UartDeviceId id, UartDevice *device);

    // 是否已初始化标志
    bool initialized_ = false;

    // 禁止拷贝构造和赋值操作
    UartBus(const UartBus &) = delete;
    UartBus &operator=(const UartBus &) = delete;

    // 使用指针数组代替固定成员变量
    UartDevice *devices_[(size_t)UartDeviceId::MAX_DEVICES] = {nullptr};

    // 实际设备实例
    UartDevice uart1_;

    UartDevice uart6_;
};

} // namespace HAL::UART