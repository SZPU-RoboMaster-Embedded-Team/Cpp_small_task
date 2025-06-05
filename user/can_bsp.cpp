#include <can_bsp.hpp>
/********************* INTERFACE_CAN_DEVICE *********************/
namespace HAL::CAN
{
    ID_t MCancDevice::extract_id(const CAN_RxHeaderTypeDef &rx_header)
    {
        return rx_header.IDE == CAN_ID_STD ? rx_header.StdId : rx_header.ExtId; //类比switch case的条件那一段
    }

/********************* INTERFACE_CAN_BUS *********************/
    MCanBus &get_can_bus_instance() //hpp里面那条函数声明的函数体
    { 
        return CanBus::instance();
    }

/********************* IMPL_CAN_DEVICE *********************/
//初始化列表法，定义构造函数
CanDevice::CanDevice(CAN_HandleTypeDef *handle, uint32_t filter_bank, uint32_t fifo)
    : handle_(handle), filter_bank_(filter_bank), fifo_(fifo), mailbox_(0)
{
}

void CanDevice::init()  //CanDevice类中成员函数的实现，函数体
{
    configure_filter();
}

void CanDevice::start()
{
    HAL_CAN_Start(handle_); //handle_是在CanDevice类中定义的成员变量，是CAN设备句柄
}

void CanDevice::start()
{
    HAL_CAN_Start(handle_);

    // 设置中断 过滤器中的内容
    if (fifo_ == CAN_FILTER_FIFO0)
    {
        HAL_CAN_ActivateNotification(handle_, CAN_IT_RX_FIFO0_MSG_PENDING);
    }
    else if (fifo_ == CAN_FILTER_FIFO1)
    {
        HAL_CAN_ActivateNotification(handle_, CAN_IT_RX_FIFO1_MSG_PENDING);
    }
}

//can_data能点出Can_data结构体的内容是因为把rx，tx这些变量引用成can_data
bool CanDevice::send(const Can_data &can_data)
{
    if (HAL_CAN_GetTxMailboxesFreeLevel(handle_) == 0)
    {
        return false;
    }

    CAN_TxHeaderTypeDef tx_header;
    tx_header.DLC = can_data.dlc;
    tx_header.IDE = can_data.is_standard_id ? CAN_ID_STD : CAN_ID_EXT;
    tx_header.RTR = can_data.is_data_frame ? CAN_RTR_DATA : CAN_RTR_REMOTE;
    uint32_t temp_mailbox = can_data.mailbox;

    if (can_data.is_standard_id)
    {
        tx_header.ExtId = 0;
        tx_header.StdId = can_data.id;
    }
    else
    {
        tx_header.StdId = 0;
        tx_header.ExtId = can_data.id;
    }

    tx_header.TransmitGlobalTime = DISABLE;

    if (HAL_CAN_AddTxMessage(handle_, &tx_header, const_cast<uint8_t *>(can_data.data), &temp_mailbox) != HAL_OK)   
    {
        return false;
    }

    return true;
}

bool CanDevice::receive(Can_data &can_data)
{
    CAN_RxHeaderTypeDef rx_header;

    if (HAL_CAN_GetRxFifoFillLevel(handle_, fifo_) == 0)    //以前没用过
    {
        return false;
    }

    if (HAL_CAN_GetRxMessage(handle_, fifo_, &rx_header, can_data.data) != HAL_OK)
    {
        return false;
    }

    // 填充can_data结构体
    can_data.id = rx_header.IDE == CAN_ID_STD ? rx_header.StdId : rx_header.ExtId;
    can_data.dlc = rx_header.DLC;
    can_data.is_standard_id = (rx_header.IDE == CAN_ID_STD);    //这里 真 是标准id
    can_data.is_data_frame = (rx_header.RTR == CAN_RTR_DATA);   //这里 真 是数据帧

    return true;
}

CAN_HandleTypeDef *CanDevice::get_handle() const
{
    return handle_;
}

void CanDevice::configure_filter()
{
    CAN_FilterTypeDef filter;
    filter.FilterActivation = CAN_FILTER_ENABLE; // 使能过滤器
    filter.FilterBank = filter_bank_;            // 通道
    filter.FilterFIFOAssignment = fifo_;         // 缓冲器
    filter.FilterIdHigh = 0x0;                   // 高16
    filter.FilterIdLow = 0x0;                    // 低16
    filter.FilterMaskIdHigh = 0x0;               // 高16
    filter.FilterMaskIdLow = 0x0;                // 低16
    filter.FilterMode = CAN_FILTERMODE_IDMASK;   // 掩码
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.SlaveStartFilterBank = 14;

    HAL_CAN_ConfigFilter(handle_, &filter);
}

/********************* IMPL_CAN_BUS *********************/
// CanBus实现
CanBus &CanBus::instance()  //&表示该函数的返回类型是一个 CanBus 类型的引用， ：：用于指定 instance 函数属于 CanBus 类。
{
    static CanBus instance; //CanBus类的实例化
    // 懒汉模式：在第一次获取实例时初始化
    if (!instance.initialized_)
    {
        instance.init();
        instance.initialized_ = true;
    }
    return instance;
}

CanBus::CanBus()
    // 初始化CAN1
    : can1_(&hcan1, 0, CAN_FILTER_FIFO1)
      // 初始化CAN2
      //can2_(&hcan2, 14, CAN_FILTER_FIFO0)
{
    // 注册现有的设备
    register_device(CanDeviceId::HAL_CAN1, &can1_);
    register_device(CanDeviceId::HAL_CAN2, &can2_);

    // 这里可以轻松注册更多设备，比如CAN3
    // 例如: register_device(CanDeviceId::HAL_Can3, &can3_);
}

void CanBus::init()
{
    // 初始化所有已注册的设备
    for (size_t i = 0; i < (size_t)CanDeviceId::MAX_DEVICES; ++i)
    {
        if (devices_[i] != nullptr)
        {
            devices_[i]->init();
            devices_[i]->start();
        }
    }
}

void CanBus::register_device(CanDeviceId id, CanDevice *device)
{
    if (id < CanDeviceId::MAX_DEVICES && device != nullptr)
    {
        devices_[(size_t)id] = device;
    }
}

MCanDevice &CanBus::get_device(CanDeviceId id)
{
    if (id < CanDeviceId::MAX_DEVICES && devices_[(size_t)id] != nullptr)
    {
        return *devices_[(size_t)id];
    }

    // 如果没有可用设备，返回can1_（保证永远有返回值）
    return can1_;
}

bool CanBus::has_device(CanDeviceId id) const
{
    return id < CanDeviceId::MAX_DEVICES && devices_[(size_t)id] != nullptr;
}
}

