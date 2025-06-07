#include "User/HAL/CAN/impl/can_device_impl.hpp"
#include "User/APP/CallBack/callback.cpp"
#include "User/HAL/CAN/impl/can_bus_impl.hpp"


#include "HAL/CAN/can_hal.hpp"

// 发送控制命令到ID为0x201的DM2006电机
void send_dm2006_command()
{
    auto& can_bus = HAL::CAN::get_can_bus_instance();

    HAL::CAN::Frame frame;
    frame.id = 0x201;             // 电机ID
    frame.dlc = 8;                // 数据长度
    frame.is_extended_id = false; // 标准帧
    frame.is_remote_frame = false;// 数据帧
    // 填充数据
    frame.data[0] = 0x01; // 具体控制协议
    frame.data[1] = 0x02;
    frame.data[2] = 0x03;
    frame.data[3] = 0x04;
    frame.data[4] = 0x05;
    frame.data[5] = 0x06;
    frame.data[6] = 0x07;
    frame.data[7] = 0x08;

    can_bus.get_can1().send(frame);
}





// CAN1 FIFO0接收中断回调
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    auto& can_bus = HAL::CAN::get_can_bus_instance();
    HAL::CAN::Frame rx_frame;

    // 判断是CAN1
    if (hcan == can_bus.get_can1().get_handle())
    {
        if (can_bus.get_can1().receive(rx_frame))
        {
            // 解析rx_frame，处理DM2006反馈
            
        }
    }
}