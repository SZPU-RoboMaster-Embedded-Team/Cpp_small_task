#include "User/BSP/Common/StateWatch/state_watch.hpp"
#include "User/HAL/CAN/can_hal.hpp"
#include "User/HAL/LOGGER/logger.hpp"
#include "User/HAL/UART/uart_hal.hpp"
#include <cstring>

uint8_t buffer[3] = {0};
auto uatr_rx_frame = HAL::UART::Data{buffer, 3};

auto device_monitor = BSP::WATCH_STATE::StateWatch("CAN1", 200);

extern "C"
{
    void Init()
    {
        HAL::CAN::get_can_bus_instance();

        auto &uart1 = HAL::UART::get_uart_bus_instance().get_device(HAL::UART::UartDeviceId::HAL_Uart1);
        uart1.receive_dma_idle(uatr_rx_frame);
        uart1.transmit(uatr_rx_frame);
    }

    void InWhile()
    {
        device_monitor.check();
    }
} // extern "C"

uint16_t speed = 0;
uint8_t data[8] = {0};

void send(uint32_t id, uint8_t *data);

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    // 获取单例
    auto &can_bus = HAL::CAN::get_can_bus_instance();
    // 参数：设备名称，超时时间(毫秒)
    // 获取CAN1的句柄
    HAL::CAN::Frame rx_frame;

    // 接收CAN1的消息
    if (hcan == can_bus.get_can1().get_handle())
    {
        can_bus.get_can1().receive(rx_frame);
        device_monitor.updateTimestamp();
    }

    // 设置发送消息
    data[(2 - 1) * 2] = speed >> 8;
    data[(2 - 1) * 2 + 1] = speed << 8 >> 8;

    // 发送消息
    send(0X1FE, data);

    auto &log = HAL::LOGGER::Logger::getInstance();

    static uint32_t count;
    count++;
    if (count == 10)
    {
        log.error("CAN1: %d\n", rx_frame.data[0]);
        log.fatal("CAN1: %d\n", rx_frame.data[0]);
        log.info("CAN1: %d\n", rx_frame.data[0]);
        log.trace("CAN1: %d\n", rx_frame.data[0]);
        log.warning("CAN1: %d\n", rx_frame.data[0]);

        count = 0;
    }
}

void send(uint32_t id, uint8_t *data)
{
    auto &can_bus = HAL::CAN::get_can_bus_instance();

    HAL::CAN::Frame tx_frame;
    tx_frame.dlc = 8;
    tx_frame.is_extended_id = false;
    tx_frame.is_remote_frame = false;
    tx_frame.id = id;

    std::memcpy(tx_frame.data, data, 8);

    can_bus.get_can1().send(tx_frame);
}

// UART中断
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    auto &uart1 = HAL::UART::get_uart_bus_instance().get_device(HAL::UART::UartDeviceId::HAL_Uart1);

    if (huart == uart1.get_handle())
    {
        uart1.transmit(uatr_rx_frame);
    }
    uart1.receive_dma_idle(uatr_rx_frame);
}
