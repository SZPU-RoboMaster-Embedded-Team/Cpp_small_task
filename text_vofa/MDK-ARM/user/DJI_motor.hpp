#ifndef _DJI_MOTOR_HPP_
#define _DJI_MOTOR_HPP_ 

#include <can.h>
#include <cstdint>
#include <cstring>

namespace BSP::Motor::DJI
{
    //要加射频
    struct Parameters
    {
        double reduction_ratio;      // 减速比
        double torque_constant;      // 力矩常数 (Nm/A)
        double feedback_current_max; // 反馈最大电流 (A)
        double current_max;          // 最大电流 (A)
        double encoder_resolution;   // 编码器分辨率

        // 自动计算的参数
        double encoder_to_deg; // 编码器值转角度系数
        double encoder_to_rpm;
        double rpm_to_radps;                    // RPM转角速度系数
        double current_to_torque_coefficient;   // 电流转扭矩系数
        double feedback_to_current_coefficient; // 反馈电流转电流系数
        double deg_to_real;                     // 角度转实际角度系数

        static constexpr double deg_to_rad = 0.017453292519611;
        static constexpr double rad_to_deg = 1 / 0.017453292519611;

         // 构造函数带参数计算
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