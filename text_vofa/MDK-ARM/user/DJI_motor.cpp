#include <can.h>
#include <cstdint>
#include <cstring>
#include <DJI_motor.hpp>

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
    
    template <uint8_t N> class DjiMotorBase : public Moto rBase<N>
    {
        protected:
            DjiMotgorBase(uint16_t Init_id, const uint8_t (&recv_idxs)[N], uint32_t send_idxs, Parameters params) : init_address(Init_id), params_(params)
            {
                //��ʼ�� recv_idxs_ �� send_idxs_
                for(uint8_t i = 0; i < N; i++)
                {
                    recv_idxs_[i] = recv_idxs[i];
                }
                send_idxs_ = send_idxs;
            }

        public:
        //����can����
        void Parse(const HAL::CAN::Frame  &frame)
        {
            const uint16_t received_id = frame.id;

            for(uint8_t i = 0; i < N; ++i)
            {
                if(received_id == init_address + recv_idxs_[i])
                {
                    memcpy(&feedback_[i], frame.data, sizeof(DjiMotorfeedback));

                    feedback_[i].angle = __builtin_bswap16(feedback_[i].angle);
                    feedback_[i].velocity = __builtin_bswap16(feedback_[i].velocity);
                    feedback_[i].current = __builtin_bswap16(feedback_[i].current);

                    configure(i);

                    this->state_watch_[i].updataTimestamp();
                }
            }
        }

        //���÷�������
        void setCAN(int16_t data, int id)
        {
            tx_frame.id = send_idxs_;
            tx_frame.dlc = 8;
            tx_frame.is_extended_id = false;
            tx_frame.is_remote_frame = false;
            tx_frame.mailbox = 0;

            tx_frame.data[(id - 1) * 2] = data >> 8;
            tx_frame.data[(id - 1) * 2 + 1] = data << 8 >> 8;
        }

        //����Can����
        void sendCAN()
        {
            auto &can = HAL::CAN::get_can_bus_instance().get_device(HAL::CAN::CanDeviceId::HAL_Can1);

            can.send(tx_frame); // ��������
        }

    protected:
        struct alignas(uint64_t) DjiMotorfeedback
        {
            int16_t angle;
            int16_t velocity;
            int16_t current;
            uint8_t temperature;
            uint8_t unused;
        };

        //��������
        // rr ���ٱ� tc ���س��� fmc �����������ֵ mc ��ʵ�������ֵ er �������ֱ���
        Parameters CreateParams(double rr, double tc, double fmc, double mc, double er) const
        {
            return Parameters(rr, tc, fmc, mc, er);
        }

    private:
        //����������ת��Ϊ���ʵ�λ
        void Configure(size_t i)
        {
            const auto &params = params_;

            this->unit_data_[i].angle_Deg = feedback_[i].angle * params.encoder_to_deg;

            this->unit_data_[i].angle_Rad = this->unit_data_[i].angle_Deg * params.deg_to_rad;

            this->unit_data_[i].velocity_Rad = feedback_[i].velocity * params.rpm_to_radps;

            this->unit_data_[i].velocity_Rpm = feedback_[i].velocity * params.encoder_to_rpm;

            this->unit_data_[i].current_A = feedback_[i].current * params.feedback_to_current_coefficient;

            this->unit_data_[i].torque_Nm = feedback_[i].current * params.current_to_torque_coefficient;

            this->unit_data_[i].temperature_C = feedback_[i].temperature;

            double lastData = this->unit_data_[i].last_angle;
            double Data = this->unit_data_[i].angle_Deg;

            if (Data - lastData < -180) // ��ת
                this->unit_data_[i].add_angle += (360 - lastData + Data) * params.deg_to_real;
            else if (Data - lastData > 180) // ��ת
                this->unit_data_[i].add_angle += -(360 - Data + lastData) * params.deg_to_real;
            else
                this->unit_data_[i].add_angle += (Data - lastData) * params.deg_to_real;

            this->unit_data_[i].last_angle = Data;
            // �Ƕȼ����߼�...
        }

        const int16_t init_address;    // ��ʼ��ַ
        DjiMotorfeedback feedback_[N]; // ��������
        uint8_t recv_idxs_[N];         // ID����
        uint32_t send_idxs_;
        HAL::CAN::Frame tx_frame;
        HAL::CAN::Frame rx_frame;

    public:
        Parameters params_; // ת���ʵ�λ�����б�
    
    }; 

    //����2006����Ĳ���
    template <uint8_t N> class GM2006 : public DjiMotorBase<N>
    {
    public:
        GM2006(uint16_t Init_id, const uint8_t (&recv_idxs)[N], uint32_t send_idxs)
            : DjiMotorBase<N>(Init_id, recv_idxs, send_idxs,
                            // ֱ�ӹ����������
                            Parameters(36.0, 0.18 / 36.0, 16384, 10, 8192))
        {
        }
    };
    
    inline GM2006<1> Motor2006(0x200, {1}, 0x200);
}