#include "can_hal.hpp"
#include <iostream>

// 使用示例

void can_example_init()
{
    // 使用懒汉模式，只需获取实例，不需要显式初始化
    // 第一次调用get_can_bus_instance()时会自动初始化
    HAL::CAN::get_can_bus_instance();
}

void can_example_send()
{
    // 创建CAN帧
    HAL::CAN::Frame frame;
    frame.id = 0x201;              // 设置ID为0x201
    frame.dlc = 8;                 // 数据长度为8字节
    frame.is_extended_id = false;  // 使用标准ID
    frame.is_remote_frame = false; // 数据帧，非远程帧

    // 设置数据
    frame.data[0] = 0x12;
    frame.data[1] = 0x34;
    frame.data[2] = 0x56;
    frame.data[3] = 0x78;
    frame.data[4] = 0x9A;
    frame.data[5] = 0xBC;
    frame.data[6] = 0xDE;
    frame.data[7] = 0xF0;

    // 通过指定CAN设备发送
    auto &can_bus = HAL::CAN::get_can_bus_instance();

    // 方法1：使用兼容旧API的方法
    can_bus.get_can1().send(frame);

    // 方法2：使用新的通用API，通过ID获取设备
    can_bus.get_device(HAL::CAN::CanDeviceId::HAL_Can2).send(frame);

    // 方法3：在发送前检查设备是否可用
    if (can_bus.has_device(HAL::CAN::CanDeviceId::HAL_Can3))
    {
        can_bus.get_device(HAL::CAN::CanDeviceId::HAL_Can3).send(frame);
    }
    else
    {
        // 设备不可用，进行错误处理
        std::cout << "CAN3 不可用" << std::endl;
    }
}

// 在中断处理函数中接收数据
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    auto &can_bus = HAL::CAN::get_can_bus_instance();

    if (hcan == can_bus.get_can1().get_handle())
    {
        // 接收CAN1的数据
        HAL::CAN::Frame rx_frame;
        if (can_bus.get_can1().receive(rx_frame))
        {
            // 处理接收到的数据
            // 例如：检查ID并处理对应的数据
            if (rx_frame.id == 0x201)
            {
                // 处理ID为0x201的数据
                // ...
            }
        }
    }
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    auto &can_bus = HAL::CAN::get_can_bus_instance();

    // 使用新的API通过ID获取设备
    if (can_bus.has_device(HAL::CAN::CanDeviceId::HAL_Can2) &&
        hcan == can_bus.get_device(HAL::CAN::CanDeviceId::HAL_Can2).get_handle())
    {
        // 接收CAN2的数据
        HAL::CAN::Frame rx_frame;
        if (can_bus.get_device(HAL::CAN::CanDeviceId::HAL_Can2).receive(rx_frame))
        {
            // 处理接收到的数据
            // ...
        }
    }
}