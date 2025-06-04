// Vofa_Send.cpp

#include "Vofa_Send.hpp"

namespace Vofa
{
    VofaSend::VofaSend(uint16_t baseId, uint16_t sendId)
        : baseId_(baseId), sendId_(sendId)
    {
    }

    void VofaSend::sendEnableMotor(uint8_t motorId, BSP::Motor::Dji::GM2006<1> &motor, PID &pid)
    {
        std::vector<uint8_t> data = {0x01, motorId}; // 假设0x01为使能命令
        sendData(data);
    }

    void VofaSend::sendDisableMotor(uint8_t motorId)
    {
        std::vector<uint8_t> data = {0x00, motorId}; // 假设0x02为失能命令
        sendData(data);
    }

    void VofaSend::sendSetSpeed(uint8_t motorId, int16_t speed)
    {
        std::vector<uint8_t> data = {0x03, motorId, static_cast<uint8_t>(speed >> 8), static_cast<uint8_t>(speed & 0xFF)}; // 假设0x03为设置速度命令
        sendData(data);
    }

    void VofaSend::parseData(const std::vector<uint8_t>& data)
    {
        if (data.size() < 2) return;

        uint8_t command = data[0];
        uint8_t motorId = data[1];

        switch (command)
        {
            case 0x00:
                // 电机使能
                // 处理使能逻辑
                break;
            case 0x01:
                // 电机失能
                // 处理失能逻辑
                break;
            case 0x03:
                if (data.size() != 4) return;
                int16_t speed = (static_cast<int16_t>(data[2]) << 8) | data[3];
                // 电机速度设置
                // 处理速度设置逻辑
                break;
            default:
                // 未知命令
                break;
        }
    }

    void VofaSend::sendData(const std::vector<uint8_t>& data)
    {
        // 这里实现具体的发送逻辑，例如通过CAN总线发送
        // 示例：
        HAL::CAN::Frame frame;
        frame.id = sendId_;
        frame.dlc = data.size();
        frame.is_extended_id = false;
        frame.is_remote_frame = false;
        frame.mailbox = 0;

        std::copy(data.begin(), data.end(), frame.data);

        auto &can = HAL::CAN::get_can_bus_instance().get_device(HAL::CAN::CanDeviceId::HAL_Can1);
        can.send(frame);
    }
}