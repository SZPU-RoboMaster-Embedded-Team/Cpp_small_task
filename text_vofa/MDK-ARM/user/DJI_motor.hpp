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
            double angle_Deg; // ��λ�ȽǶ�
            double angle_Rad; // ��λ���ȽǶ�

            double velocity_Rad; // ��λ�����ٶ�
            double velocity_Rpm; // ��λrpm

            double current_A;     // ��λ����
            double torque_Nm;     // ��λţ��
            double temperature_C; // ��λ���϶�

            double last_angle;
            double add_angle;
        };

        //���ʵ�λ����
        UnitData unit_data_[N];
        //�豸���߼��
        BSP::WATCH_STATE::StateWatch state_watch_[N];

        virtual void Pare(const HAL::CAN::Frame &frame) = 0;

    public:
        //��ȡ�Ƕ�
        float getAngleDeg(uint8_t id)
        {
            return this->unit_data_[id - 1].angle_Deg;
        }

        //��ȡ����
        float getAngleRad(uint8_t id)
        {
            return this->unit_data_[id - 1].angle_Rad;
        }

        //��ȡ��һ�νǶ�
        float getLastAngleDeg(uint8_t id)
        {
            return this->unit_data_[id - 1].last_angle;
        }
        
        //��ȡ�����Ƕ�
        float getAddAngleDeg(uint8_t id)
        {
            return this->unit_data_[id - 1].add_angle;
        }
        
        //��ȡ��������
        float getAddAngleRad(uint8_t id)
        {
            return this->unit_data_[id - 1].add_angle;
        }

        //��ȡ�ٶ�    ��λ��(rad/s) ת��
        float getVelocityRads(uint8_t id)
        {
            return this->unit_data_[id - 1].velocity_Rad;
        }

        //��ȡ�ٶ�    ��λ��(rpm) ת�� 
        float getVelocityRpm(uint8_t id)
        {
            return this->unit_data_[id - 1].velocity_Rpm;
        }
        
        //��ȡ����ֵ    ��λ��(A)
        float getCelocityRpm(uint8_t id)
        {
            return this->unit_data_[id - 1].velocity_Rpm;
        }

        float getCurrent(uint8_t id)
        {
            return this->unit_data_[id - 1].current_A;
        }

        //��ȡ����    ��λ��(Nm)
        float getTorque(uint8_t id)
        {
            return this->unit_data_[id - 1].torque_Nm;
        }

        //��ȡ�¶�    ��λ��(��)
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
                    return i + 1; // ���ص��ߵ���ı�ţ���1��ʼ������
                }
            }

            return 0; // ���е��������
        }
};
}
#endif