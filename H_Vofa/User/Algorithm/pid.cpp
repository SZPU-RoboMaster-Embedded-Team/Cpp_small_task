/**
 * @file pid.cpp
 * @author XMX
 * @brief PID¿‡∑Ω∑®
 * @version 1.0
 * @date 2024-08-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "pid.hpp"

float PID::clamp(float value, float min, float max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

float PID::compute(float target, float measure)
{
    error_ = target - measure;
    integral_ += error_;
    integral_ = clamp(integral_, -integral_limit_, integral_limit_);
    derivative_ = error_ - prev_error_;
    prev_error_ = error_;
    output_ = kp_ * error_ + ki_ * integral_ + kd_ * derivative_;
    output_ = clamp(output_, -output_limit_, output_limit_);
    return output_;
}

float PID::computeError(float error)
{
    error_ = error;
    integral_ += error_;
    integral_ = clamp(integral_, -integral_limit_, integral_limit_);
    derivative_ = error_ - prev_error_;
    prev_error_ = error_;
    output_ = kp_ * error + ki_ * integral_ + kd_ * derivative_;
    output_ = clamp(output_, -output_limit_, output_limit_);
    return output_;
}
