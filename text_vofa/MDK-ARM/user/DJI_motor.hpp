#ifndef _DJI_MOTOR_HPP_
#define _DJI_MOTOR_HPP_ 

#include <can.h>
#include <cstdint>
#include <cstring>

namespace BSP::Motor::DJI
{
    //Ҫ����Ƶ
    struct Parameters
    {
        double reduction_ratio;      // ���ٱ�
        double torque_constant;      // ���س��� (Nm/A)
        double feedback_current_max; // ���������� (A)
        double current_max;          // ������ (A)
        double encoder_resolution;   // �������ֱ���

        // �Զ�����Ĳ���
        double encoder_to_deg; // ������ֵת�Ƕ�ϵ��
        double encoder_to_rpm;
        double rpm_to_radps;                    // RPMת���ٶ�ϵ��
        double current_to_torque_coefficient;   // ����תŤ��ϵ��
        double feedback_to_current_coefficient; // ��������ת����ϵ��
        double deg_to_real;                     // �Ƕ�תʵ�ʽǶ�ϵ��

        static constexpr double deg_to_rad = 0.017453292519611;
        static constexpr double rad_to_deg = 1 / 0.017453292519611;

         // ���캯������������
        Parameters(double rr, double tc, double fmc, double mc, double er)
            : reduction_ratio(rr), torque_constant(tc), feedback_current_max(fmc), current_max(mc), encoder_resolution(er)
        {
            constexpr double PI = 3.14159265358979323846;
            encoder_to_deg = 360.0 / encoder_resolution;
            rpm_to_radps = 1 / reduction_ratio / 60 * 2 * PI;
            encoder_to_rpm = 1 / reduction_ratio;
            current_to_torque_coefficient = reduction_ratio * torque_constant / feedback_current_max * current_max;
            feedback_to_current_coefficient = current_max / feedback_current_max;
            deg_to_real = 1 / reduction_ratio;
        }
    };

    
}

#endif