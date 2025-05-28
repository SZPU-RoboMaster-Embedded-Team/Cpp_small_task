#pragma once

class PID
{
  public:
    PID(float kp, float ki, float kd, float integral_limit, float output_limit)
        : kp_(kp), ki_(ki), kd_(kd), integral_limit_(integral_limit), output_limit_(output_limit)
    {
    }

    // 设置参数
    void setKp(float kp)
    {
        kp_ = kp;
    }
    void setKi(float ki)
    {
        ki_ = ki;
    }
    void setKd(float kd)
    {
        kd_ = kd;
    }
    void setIntegralLimit(float limit)
    {
        integral_limit_ = limit;
    }
    void setOutputLimit(float limit)
    {
        output_limit_ = limit;
    }

    // 获取参数
    float getKp() const
    {
        return kp_;
    }
    float getKi() const
    {
        return ki_;
    }
    float getKd() const
    {
        return kd_;
    }
    float getIntegralLimit() const
    {
        return integral_limit_;
    }
    float getOutputLimit() const
    {
        return output_limit_;
    }

    // 清零
    void reset()
    {
        error_ = 0.0f;
        prev_error_ = 0.0f;
        integral_ = 0.0f;
        derivative_ = 0.0f;
        output_ = 0.0f;
    }

    // 主计算接口
    float compute(float target, float measure);
    float computeError(float error);

    // 获取内部状态
    float getError() const
    {
        return error_;
    }
    float getIntegral() const
    {
        return integral_;
    }
    float getDerivative() const
    {
        return derivative_;
    }
    float getOutput() const
    {
        return output_;
    }

  private:
    float kp_{0.0f};
    float ki_{0.0f};
    float kd_{0.0f};
    float error_{0.0f};
    float prev_error_{0.0f};
    float integral_{0.0f};
    float derivative_{0.0f};
    float output_{0.0f};
    float integral_limit_{0.0f};
    float output_limit_{0.0f};

    static float clamp(float value, float min, float max);
};
