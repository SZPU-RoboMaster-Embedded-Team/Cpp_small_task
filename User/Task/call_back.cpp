#include "../Task/call_back.hpp"
#include "../APP/variables.hpp" // 加入PID变量声明
#include "../BSP/Motor/DM/DmMotor.hpp"
#include "../BSP/Motor/Dji/DjiMotor.hpp"
#include "../BSP/Remote/Dbus/Dbus.hpp"
#include "../BSP/Vofa/Vofa_send.hpp"

// can_filo0中断接收
CAN_RxHeaderTypeDef CAN1_RxHeader; // can接收数据
uint8_t CAN1_RxHeaderData[8] = {0};
static float time_axis = 0.0f; // 时间轴
static uint32_t last_tick = 0;
uint32_t now_tick = HAL_GetTick();
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    // 接受信息
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &CAN1_RxHeader, CAN1_RxHeaderData);
    if (hcan == &hcan1)
    {
        BSP::Motor::Dji::Motor6020.Parse(CAN1_RxHeader, CAN1_RxHeaderData);
        BSP::Motor::Dji::Motor3508.Parse(CAN1_RxHeader, CAN1_RxHeaderData);
        BSP::Motor::Dji::Motor2006.Parse(CAN1_RxHeader, CAN1_RxHeaderData);
    }
}
void Vofa_Motor2006_Control()
{
    float dt = (now_tick - last_tick) / 1000.0f;
    last_tick = now_tick;
    vofaRecv(); // 先接收数据，更新 vofa_target_speed 和 vofa_enable_flag

    // 获取2006电机反馈数据
    const auto &feedback = BSP::Motor::Dji::Motor2006.GetUnitData(0);
    if (vofa_enable_flag == 0x01)
        time_axis += dt;
    else
        time_axis = 0.0f;
    float freq = (feedback.velocity_Rpm / 60.0f) / 36.0f * 9.0f;
    // 发送角度、速度、电流等到vofa
    vofaSend(feedback.angle_Deg, feedback.velocity_Rpm, time_axis, freq, 0, 0);

    // 控制电机
    if (vofa_enable_flag == 0x01)
    {
        // 使用PID计算期望电流或速度
        float pid_output = pid_vel_204.compute(vofa_target_speed, feedback.velocity_Rpm);
        BSP::Motor::Dji::Motor2006.setCAN((int16_t)pid_output, 4);
        BSP::Motor::Dji::Motor2006.sendCAN(&hcan1, 0);
    }
    else
    {
        // 关闭2006电机
        BSP::Motor::Dji::Motor2006.setCAN(0, 4);
        BSP::Motor::Dji::Motor2006.sendCAN(&hcan1, 0);
    }
}