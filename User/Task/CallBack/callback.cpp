#include "User/HAL/CAN/can_hal.hpp"
#include <cstring>

extern "C"
{
    void Init()
    {
        auto &can_bus = HAL::CAN::get_can_bus_instance();
        can_bus.get_can1().init();
    }
} // extern "C"

uint16_t speed = 0;
uint8_t data[8] = {0};

void send(uint32_t id, uint8_t *data);

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    // 获取单例
    auto &can_bus = HAL::CAN::get_can_bus_instance();
    // 获取CAN1的句柄
    HAL::CAN::Frame rx_frame;

    // 接收CAN1的消息
    if (hcan == can_bus.get_can1().get_handle())
    {
        can_bus.get_can1().receive(rx_frame);
    }

    // 设置发送消息
    data[(2 - 1) * 2] = speed >> 8;
    data[(2 - 1) * 2 + 1] = speed << 8 >> 8;

    // 发送消息
    send(0X1FE, data);
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