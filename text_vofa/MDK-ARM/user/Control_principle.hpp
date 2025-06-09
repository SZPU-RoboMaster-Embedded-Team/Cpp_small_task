namespace CONTROL::PID
{
    class PIDController {
    public:
        PIDController(float kp, float ki, float kd, float max_out, float max_iout, float max_error_i)
            : kp(kp), ki(ki), kd(kd), max_out(max_out), max_iout(max_iout), max_error_i(max_error_i) 
        {
            error[0] = error[1] = error[2] = 0.0f;
            out = p_out = i_out = d_out = 0.0f;
        }

        float calculate(float ref, float set) 
        {
            error[2] = error[1];
            error[1] = error[0];
            error[0] = set - ref;

            // P
            p_out = kp * error[0];

            // I
            if (error[0] < max_error_i) {
                i_out += ki * error[0];
                limitMax(i_out, max_iout);
            } else {
                i_out = 0.0f;
            }

            // D
            d_out = kd * (error[0] - error[1]);

            out = p_out + i_out + d_out;
            limitMax(out, max_out);

            return out;
        }

    private:
        float kp, ki, kd;
        float max_out, max_iout, max_error_i;
        float error[3];
        float out, p_out, i_out, d_out;

        void limitMax(float& value, float max) 
        {
            if (value > max) value = max;
            else if (value < -max) value = -max;
        }
    };
}
