#include "User/HAL/CAN/can_hal.hpp"

extern "C"
{
    void Init()
    {
        auto &can_bus = HAL::CAN::get_can_bus_instance();
    }
} // extern "C"

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    auto &can_bus = HAL::CAN::get_can_bus_instance();
    HAL::CAN::Frame rx_frame;
    if (hcan == can_bus.get_can1().get_handle())
    {
        can_bus.get_can1().receive(rx_frame);
    }
}
