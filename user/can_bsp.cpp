#include <can_bsp.hpp>
/********************* INTERFACE_CAN_DEVICE *********************/
namespace HAL::CAN
{
    ID_t MCancDevice::extract_id(const CAN_RxHeaderTypeDef &rx_header)
    {
        return rx_header.IDE == CAN_ID_STD ? rx_header.StdId : rx_header.ExtId; //���switch case��������һ��
    }

/********************* INTERFACE_CAN_BUS *********************/
    MCanBus &get_can_bus_instance() //hpp�����������������ĺ�����
    { 
        return CanBus::instance();
    }

/********************* IMPL_CAN_DEVICE *********************/
//��ʼ���б������幹�캯��
CanDevice::CanDevice(CAN_HandleTypeDef *handle, uint32_t filter_bank, uint32_t fifo)
    : handle_(handle), filter_bank_(filter_bank), fifo_(fifo), mailbox_(0)
{
}

void CanDevice::init()  //CanDevice���г�Ա������ʵ�֣�������
{
    configure_filter();
}

void CanDevice::start()
{
    HAL_CAN_Start(handle_); //handle_����CanDevice���ж���ĳ�Ա��������CAN�豸���
}

void CanDevice::start()
{
    HAL_CAN_Start(handle_);

    // �����ж� �������е�����
    if (fifo_ == CAN_FILTER_FIFO0)
    {
        HAL_CAN_ActivateNotification(handle_, CAN_IT_RX_FIFO0_MSG_PENDING);
    }
    else if (fifo_ == CAN_FILTER_FIFO1)
    {
        HAL_CAN_ActivateNotification(handle_, CAN_IT_RX_FIFO1_MSG_PENDING);
    }
}

//can_data�ܵ��Can_data�ṹ�����������Ϊ��rx��tx��Щ�������ó�can_data
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

    if (HAL_CAN_GetRxFifoFillLevel(handle_, fifo_) == 0)    //��ǰû�ù�
    {
        return false;
    }

    if (HAL_CAN_GetRxMessage(handle_, fifo_, &rx_header, can_data.data) != HAL_OK)
    {
        return false;
    }

    // ���can_data�ṹ��
    can_data.id = rx_header.IDE == CAN_ID_STD ? rx_header.StdId : rx_header.ExtId;
    can_data.dlc = rx_header.DLC;
    can_data.is_standard_id = (rx_header.IDE == CAN_ID_STD);    //���� �� �Ǳ�׼id
    can_data.is_data_frame = (rx_header.RTR == CAN_RTR_DATA);   //���� �� ������֡

    return true;
}

CAN_HandleTypeDef *CanDevice::get_handle() const
{
    return handle_;
}

void CanDevice::configure_filter()
{
    CAN_FilterTypeDef filter;
    filter.FilterActivation = CAN_FILTER_ENABLE; // ʹ�ܹ�����
    filter.FilterBank = filter_bank_;            // ͨ��
    filter.FilterFIFOAssignment = fifo_;         // ������
    filter.FilterIdHigh = 0x0;                   // ��16
    filter.FilterIdLow = 0x0;                    // ��16
    filter.FilterMaskIdHigh = 0x0;               // ��16
    filter.FilterMaskIdLow = 0x0;                // ��16
    filter.FilterMode = CAN_FILTERMODE_IDMASK;   // ����
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.SlaveStartFilterBank = 14;

    HAL_CAN_ConfigFilter(handle_, &filter);
}

/********************* IMPL_CAN_BUS *********************/
// CanBusʵ��
CanBus &CanBus::instance()  //&��ʾ�ú����ķ���������һ�� CanBus ���͵����ã� ��������ָ�� instance �������� CanBus �ࡣ
{
    static CanBus instance; //CanBus���ʵ����
    // ����ģʽ���ڵ�һ�λ�ȡʵ��ʱ��ʼ��
    if (!instance.initialized_)
    {
        instance.init();
        instance.initialized_ = true;
    }
    return instance;
}

CanBus::CanBus()
    // ��ʼ��CAN1
    : can1_(&hcan1, 0, CAN_FILTER_FIFO1)
      // ��ʼ��CAN2
      //can2_(&hcan2, 14, CAN_FILTER_FIFO0)
{
    // ע�����е��豸
    register_device(CanDeviceId::HAL_CAN1, &can1_);
    register_device(CanDeviceId::HAL_CAN2, &can2_);

    // �����������ע������豸������CAN3
    // ����: register_device(CanDeviceId::HAL_Can3, &can3_);
}

void CanBus::init()
{
    // ��ʼ��������ע����豸
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

    // ���û�п����豸������can1_����֤��Զ�з���ֵ��
    return can1_;
}

bool CanBus::has_device(CanDeviceId id) const
{
    return id < CanDeviceId::MAX_DEVICES && devices_[(size_t)id] != nullptr;
}
}

