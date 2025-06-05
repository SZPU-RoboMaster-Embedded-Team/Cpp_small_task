#ifndef _CAN_BSP_HPP_
#define _CAN_BSP_HPP_
#include  <can.h>

/********************* INTERFACE_CAN_DEVICE *********************/
namespace HAL::CAN
{
    
    using ID_t = uint32_t;  //把uint32_t写成ID_t，更直观

    struct Can_data
    {
        uint8_t data[8];        //发送or接收的数据
        ID_t id;                //发送or接收的ID
        uint32_t dlc;           //发送的DLC
        uint32_t mailbox;       //发送的邮箱
        bool is_standard_id;    //发送是否使用标准帧CAN_ID_STD 是
        bool is_data_frame;     //发送是否使用数据帧CAN_RTR_DATA 是
    };
    
    class MCanDevice
    {
        public:
            virtual ~MCanDevice() = default;
            virtual  void init() = 0;   //初始化CAN 多态
            virtual  void start() = 0;  //启动CAN
            virtual  bool  send(const Can_data  &can_data) = 0; //是否成功发送数据
            virtual  bool  receive(Can_data  &can_data) = 0;    //是否成功接收数据
            virtual CAN_HandleTypeDef *get_handle() const = 0;  //获取CAN句柄？？？
            static ID_t extract_id(const CAN_RxHeaderTypeDef &rx_header);   //从接收帧头中提取ID，静态函数
    };

/********************* INTERFACE_CAN_BUS *********************/
    enum class CanDeviceId : uint8_t    //强类型枚举？？？
    {
        HAL_CAN1 = 0,
        HAL_CAN2 = 1,
        MAX_DEVICES 
    };

    class MCanBus
    {
        public:

            virtual ~MCanBus() = default;
            
            virtual MCanDevice &get_device(CanDeviceId id) = 0;    //返回一个MCancDevice类型的值，传进去是枚举CanDeviceId的值
            MCanDevice &get_can1()
            {
                return get_device(CanDeviceId::HAL_CAN1);   //这是个成员函数，返回的东西和上面那个纯虚函数是一样的，只是实例化了
            }

            MCanDevice &get_can2()
            {
                return get_device(CanDeviceId::HAL_CAN2);
            }

            virtual bool has_device(CanDeviceId id) const = 0;  //判断是否有这个设备
    };

    MCanBus  &get_can_bus_instance();   //获取CAN总线实例 函数的声明？？？

/********************* IMPL_CAN_DEVICE *********************/
    class CanDevice :  public MCanDevice
    {
        public:
            //显示构造函数，初始化can设备，后面两个参数是滤波器的东西？
            explicit CanDevice(CAN_HandleTypeDef *handle, uint32_t filter_bank, uint32_t fifo);
            ~CanDevice() override = default;

            // 禁止拷贝构造和赋值操作
            CanDevice(const CanDevice &) = delete;
            CanDevice &operator=(const CanDevice &) = delete;

            // 实现MCanDevice接口,成员函数的声明
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

        //配置过滤器
            void configure_filter();
    };

/********************* IMPL_CAN_BUS *********************/
// CAN总线管理实现类
    class CanBus : public MCanBus
    {
    public:
        // 获取单例实例
        static CanBus &instance();  //静态函数

        // 析构函数
        ~CanBus() override = default;

        // 实现MCanBus接口
        MCanDevice &get_device(CanDeviceId id) override;
        bool has_device(CanDeviceId id) const override;

        // 初始化CAN总线（私有，由instance()调用）
        void init();

    private:
        // 私有构造函数（单例模式）
        CanBus();

        // 注册一个CAN设备
        void register_device(CanDeviceId id, CanDevice *device);

        // 是否已初始化标志
        bool initialized_ = false;

        // 禁止拷贝构造和赋值操作
        CanBus(const CanBus &) = delete;
        CanBus &operator=(const CanBus &) = delete;

        // 使用指针数组代替固定成员变量
        CanDevice *devices_[(size_t)CanDeviceId::MAX_DEVICES] = {nullptr};

        // 实际设备实例
        CanDevice can1_;
        CanDevice can2_;
    };
}



#endif

