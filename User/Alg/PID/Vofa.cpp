#include "User/HAL/CAN/impl/can_device_impl.hpp"
#include "User/APP/CallBack/callback.cpp"
#include "User/HAL/CAN/impl/can_bus_impl.hpp"


auto& can_bus = HAL::CAN::get_can_bus_instance();
HAL::CAN::Frame frame;
frame.id = 0x201; // 目标电机ID
frame.dlc = 8;
frame.is_extended_id = false;
frame.is_remote_frame = false;
// 填充 frame.data
can_bus.get_can1().send(frame);


void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    auto& can_bus = HAL::CAN::get_can_bus_instance();
    HAL::CAN::Frame rx_frame;
    if (hcan == can_bus.get_can1().get_handle())
    {
        can_bus.get_can1().receive(rx_frame);
        // 解析 rx_frame，处理电机反馈
    }
}