#ifndef _DJI_MOTOR_HPP_
#define _DJI_MOTOR_HPP_ 


#include <hal_can.hpp>
namespace BSP::Motor
{
template <uint8_t N> class MotorBase
{
    protected:
        struct UnitData
        {
            double angle_Deg; // 单位度角度
            double angle_Rad; // 单位弧度角度

            double velocity_Rad; // 单位弧度速度
            double velocity_Rpm; // 单位rpm

            double current_A;     // 单位安培
            double torque_Nm;     // 单位牛米
            double temperature_C; // 单位摄氏度

            double last_angle;
            double add_angle;
        };

        //国际单位数据
        UnitData unit_data_[N];
        //设备在线监测
        BSP::WATCH_STATE::StateWatch state_watch_[N];

        virtual void Pare(const HAL::CAN::Frame &frame) = 0;

    public:
        //获取角度
        float getAngleDeg(uint8_t id)
        {
            return this->unit_data_[id - 1].angle_Deg;
        }

        //获取弧度
        float getAngleRad(uint8_t id)
        {
            return this->unit_data_[id - 1].angle_Rad;
        }

        //获取上一次角度
        float getLastAngleDeg(uint8_t id)
        {
            return this->unit_data_[id - 1].last_angle;
        }
        
        //获取增量角度
        float getAddAngleDeg(uint8_t id)
        {
            return this->unit_data_[id - 1].add_angle;
        }
        
        //获取增量弧度
        float getAddAngleRad(uint8_t id)
        {
            return this->unit_data_[id - 1].add_angle;
        }

        //获取速度    单位：(rad/s) 转轴
        float getVelocityRads(uint8_t id)
        {
            return this->unit_data_[id - 1].velocity_Rad;
        }

        //获取速度    单位：(rpm) 转子 
        float getVelocityRpm(uint8_t id)
        {
            return this->unit_data_[id - 1].velocity_Rpm;
        }
        
        //获取电流值    单位：(A)
        float getCelocityRpm(uint8_t id)
        {
            return this->unit_data_[id - 1].velocity_Rpm;
        }

        float getCurrent(uint8_t id)
        {
            return this->unit_data_[id - 1].current_A;
        }

        //获取力矩    单位：(Nm)
        float getTorque(uint8_t id)
        {
            return this->unit_data_[id - 1].torque_Nm;
        }

        //获取温度    单位：(°)
        float getTemperature(uint8_t id)
        {
            return this->unit_data_[id - 1].temperature_C;
        }

        uint8_t getOfflineStatus()
        {
            for (uint8_t i = 0; i < N; i++)
            {
                if (this->state_watch_[i].getStatus() != BSP::WATCH_STATE::Status::ONLINE)
                {
                    return i + 1; // 返回掉线电机的编号（从1开始计数）
                }
            }

            return 0; // 所有电机都在线
        }
};
}

#endif