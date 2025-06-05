#ifndef _CAN_BSP_HPP_
#define _CAN_BSP_HPP_
#include  <can.h>

/********************* INTERFACE_CAN_DEVICE *********************/
namespace HAL::CAN
{
    
    using ID_t = uint32_t;  //��uint32_tд��ID_t����ֱ��

    struct Can_data
    {
        uint8_t data[8];        //����or���յ�����
        ID_t id;                //����or���յ�ID
        uint32_t dlc;           //���͵�DLC
        uint32_t mailbox;       //���͵�����
        bool is_standard_id;    //�����Ƿ�ʹ�ñ�׼֡CAN_ID_STD ��
        bool is_data_frame;     //�����Ƿ�ʹ������֡CAN_RTR_DATA ��
    };
    
    class MCanDevice
    {
        public:
            virtual ~MCanDevice() = default;
            virtual  void init() = 0;   //��ʼ��CAN ��̬
            virtual  void start() = 0;  //����CAN
            virtual  bool  send(const Can_data  &can_data) = 0; //�Ƿ�ɹ���������
            virtual  bool  receive(Can_data  &can_data) = 0;    //�Ƿ�ɹ���������
            virtual CAN_HandleTypeDef *get_handle() const = 0;  //��ȡCAN���������
            static ID_t extract_id(const CAN_RxHeaderTypeDef &rx_header);   //�ӽ���֡ͷ����ȡID����̬����
    };

/********************* INTERFACE_CAN_BUS *********************/
    enum class CanDeviceId : uint8_t    //ǿ����ö�٣�����
    {
        HAL_CAN1 = 0,
        HAL_CAN2 = 1,
        MAX_DEVICES 
    };

    class MCanBus
    {
        public:

            virtual ~MCanBus() = default;
            
            virtual MCanDevice &get_device(CanDeviceId id) = 0;    //����һ��MCancDevice���͵�ֵ������ȥ��ö��CanDeviceId��ֵ
            MCanDevice &get_can1()
            {
                return get_device(CanDeviceId::HAL_CAN1);   //���Ǹ���Ա���������صĶ����������Ǹ����麯����һ���ģ�ֻ��ʵ������
            }

            MCanDevice &get_can2()
            {
                return get_device(CanDeviceId::HAL_CAN2);
            }

            virtual bool has_device(CanDeviceId id) const = 0;  //�ж��Ƿ�������豸
    };

    MCanBus  &get_can_bus_instance();   //��ȡCAN����ʵ�� ����������������

/********************* IMPL_CAN_DEVICE *********************/
    class CanDevice :  public MCanDevice
    {
        public:
            //��ʾ���캯������ʼ��can�豸�����������������˲����Ķ�����
            explicit CanDevice(CAN_HandleTypeDef *handle, uint32_t filter_bank, uint32_t fifo);
            ~CanDevice() override = default;

            // ��ֹ��������͸�ֵ����
            CanDevice(const CanDevice &) = delete;
            CanDevice &operator=(const CanDevice &) = delete;

            // ʵ��MCanDevice�ӿ�,��Ա����������
            void init() override;
            void start() override;
            bool send(const Can_data &can_data) override;
            bool receive(Can_data &can_data) override;
            CAN_HandleTypeDef *get_handle() const override;

        private:
            CAN_HandleTypeDef *handle_;
            uint32_t filter_bank_;
            uint32_t fifo_;
            uint32_t mailbox_;

        //���ù�����
            void configure_filter();
    };

/********************* IMPL_CAN_BUS *********************/
// CAN���߹���ʵ����
    class CanBus : public MCanBus
    {
    public:
        // ��ȡ����ʵ��
        static CanBus &instance();  //��̬����

        // ��������
        ~CanBus() override = default;

        // ʵ��MCanBus�ӿ�
        MCanDevice &get_device(CanDeviceId id) override;
        bool has_device(CanDeviceId id) const override;

        // ��ʼ��CAN���ߣ�˽�У���instance()���ã�
        void init();

    private:
        // ˽�й��캯��������ģʽ��
        CanBus();

        // ע��һ��CAN�豸
        void register_device(CanDeviceId id, CanDevice *device);

        // �Ƿ��ѳ�ʼ����־
        bool initialized_ = false;

        // ��ֹ��������͸�ֵ����
        CanBus(const CanBus &) = delete;
        CanBus &operator=(const CanBus &) = delete;

        // ʹ��ָ���������̶���Ա����
        CanDevice *devices_[(size_t)CanDeviceId::MAX_DEVICES] = {nullptr};

        // ʵ���豸ʵ��
        CanDevice can1_;
        CanDevice can2_;
    };
}



#endif

