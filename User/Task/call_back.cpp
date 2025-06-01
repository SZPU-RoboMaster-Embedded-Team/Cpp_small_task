#include "../APP/variables.hpp"
#include "../BSP/CAN_BSP/can_bus_impl.hpp"
#include "../BSP/Motor/Dji/DjiMotor.hpp"
#include "../BSP/UART_BSP/uart_bus_impl.hpp"
#include "../BSP/Vofa/Vofa_send.hpp"

// 全局缓冲区
uint8_t rx_buf[8] = {0};

// 获取UART6设备
auto &uart6_dev = HAL::UART::UartBus::instance().get_device(HAL::UART::UartDeviceId::HAL_Uart6);

// vofa控制器
VofaMotorController vofaMotor(&uart6_dev, &BSP::Motor::Dji::Motor2006, pid_vel_204);

// CAN接收回调（由HAL库调用，但只做数据转发）
CAN_RxHeaderTypeDef CAN1_RxHeader;
uint8_t CAN1_RxHeaderData[8] = {0};

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    // 只做数据转发，解析交给Motor
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &CAN1_RxHeader, CAN1_RxHeaderData);
    if (hcan == &hcan1)
    {
        BSP::Motor::Dji::Motor2006.Parse(CAN1_RxHeader, CAN1_RxHeaderData);
    }
}

// UART接收回调（由HAL库调用，但只做数据转发）
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart6)
    {
        if (rx_buf[0] == 0xA1)
            vofaMotor.setTargetSpeed(*(float *)&rx_buf[1]);
        else if (rx_buf[0] == 0xA2)
            vofaMotor.setEnableFlag(rx_buf[1]);

        // 重新启动接收（如有BSP层接收API可替换为BSP层）
        HAL_UART_Receive_IT(&huart6, rx_buf, 5);
    }
}
#ifdef __cplusplus
extern "C"
{
#endif
    // 主控循环
    void Vofa_Motor_Control()
    {
        vofaMotor.process();
        auto feedback = BSP::Motor::Dji::Motor2006.GetUnitData(0);
    }
#ifdef __cplusplus
}
#endif