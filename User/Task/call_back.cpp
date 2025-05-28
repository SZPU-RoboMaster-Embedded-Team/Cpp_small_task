#include "../Task/call_back.hpp"
#include "../APP/variables.hpp"
#include "../BSP/Motor/DM/DmMotor.hpp"
#include "../BSP/Motor/Dji/DjiMotor.hpp"

#include "../BSP/Vofa/Vofa_send.hpp"

// can_filo0中断接收
CAN_RxHeaderTypeDef CAN1_RxHeader; // can接收数据
uint8_t CAN1_RxHeaderData[8] = {0};
static float time_axis = 0.0f; // 时间轴
static uint32_t last_tick = 0;
uint32_t now_tick = HAL_GetTick();
VofaMotorController vofaMotor(&huart6, &BSP::Motor::Dji::Motor2006, pid_vel_204);
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    // 接受信息
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &CAN1_RxHeader, CAN1_RxHeaderData);
    if (hcan == &hcan1)
    {
        // BSP::Motor::Dji::Motor6020.Parse(CAN1_RxHeader, CAN1_RxHeaderData);
        // BSP::Motor::Dji::Motor3508.Parse(CAN1_RxHeader, CAN1_RxHeaderData);
        BSP::Motor::Dji::Motor2006.Parse(CAN1_RxHeader, CAN1_RxHeaderData);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart6)
    {
        // 处理数据
        if (rx_buf[0] == 0xA1)
            vofaMotor.setTargetSpeed(*(float *)&rx_buf[1]);
        else if (rx_buf[0] == 0xA2)
            vofaMotor.setEnableFlag(rx_buf[1]);

        // 重新启动接收
        HAL_UART_Receive_IT(&huart6, rx_buf, 5);
    }
}

void Vofa_Motor_Control()
{
    vofaMotor.process();
    auto feedback = BSP::Motor::Dji::Motor2006.GetUnitData(0);
}