#ifndef _HAL_SERIAL_PORT_HPP_
#define _HAL_SERIAL_PORT_HPP_

#include <math.h>
#include <usart.h>

/********************* INTERFACE_UART_DEVICE *********************/
namespace HAL::UART
{
    // UART鏁版嵁缁撴瀯浣�
    struct Data
    {
        uint8_t *buffer; // 鏁版嵁缂撳啿鍖烘寚閽�
        uint16_t size;   // 鏁版嵁澶у皬
    };

    // UART璁惧鎶借薄鎺ュ彛
    class IUartDevice
    {
        public:
            virtual ~IUartDevice() = default;

            // 鍒濆鍖朥ART璁惧
            virtual void init() = 0;

            //鍚姩UART璁惧
            virtual void start() = 0;

            // 鍙戦€佹暟鎹紙闈為樆濉烇級
            virtual bool transmit(const Data &data) = 0;

            //鎺ユ敹鏁版嵁锛堥潪闃诲锛�
            virtual bool receive(Data &data) = 0;
            
            //鑾峰彇UART璁惧鍙ユ焺
            virtual UART_HandleTypeDef *get_handle() const = 0;

            //鍙戦€佸崟涓瓧鑺�
            virtual bool transmit_byte(uint8_t byte) = 0;

            //鎺ユ敹鍗曚釜瀛楄妭
            virtual bool receive_byte(uint8_t &byte) = 0;

            //浣跨敤DMA鍙戦€佹暟鎹�
            virtual bool transmit_dma(const Data &data) = 0;

            // 浣跨敤DMA鎺ユ敹鏁版嵁
            virtual bool receive_dma(Data &data) = 0;

            //璁剧疆DMA杩炵画鎺ユ敹骞朵娇鐢ㄧ┖闂蹭腑鏂娴�
            virtual bool receive_dma_idle(Data &data) = 0;

            //娓呮ORE閿欒锛堜覆鍙ｆ孩鍑猴級骞堕噸鏂板惎鍔―MA鎺ユ敹
            virtual void clear_ore_error(Data &data) = 0;
    };

/********************* INTERFACE_UART_BUS *********************/
// UART璁惧ID鏋氫妇
enum class UartDeviceId : uint8_t
{
    HAL_UART1 = 0,
    HAL_UART2 = 1,
    HAL_UART3 = 2,
    HAL_UART4 = 3,
    HAL_UART5 = 4,
    HAL_UART6 = 5,
    HAL_UART7 = 6,
    HAL_UART8 = 7,
    MAX_DEVICES
};

//uart鎬荤嚎鎶借薄鎺ュ彛
class IUartBus
{
    public:
        virtual ~IUartBus() = default;

        // 鑾峰彇鎸囧畾UART璁惧
        virtual IUartDevice *get_device(UartDeviceId id) = 0;

        //鍏煎鏃PI锛堝簲鐢ㄧ▼搴忕紪绋嬫帴鍙ｏ級鐨勪究鎹锋柟娉�
        // IUartDevice &get_uart1()
        // {
        //     return get_device(UartDeviceId::HAL_UART1);
        // }
        // IUartDevice &get_uart2()
        // {
        //     return get_device(UartDeviceId::HAL_UART2);
        // }
        // IUartDevice &get_uart3()
        // {
        //     return get_device(UartDeviceId::HAL_UART3);
        // }
        // IUartDevice &get_uart4()
        // {
        //     return get_device(UartDeviceId::HAL_UART4);
        // }
        // IUartDevice &get_uart5()
        // {
        //     return get_device(UartDeviceId::HAL_UART5);
        // }
        // IUartDevice &get_uart6()
        // {
        //     return get_device(UartDeviceId::HAL_UART6);
        // }
        // IUartDevice &get_uart7()
        // {
        //     return get_device(UartDeviceId::HAL_UART7);
        // }
        // IUartDevice &get_uart8()
        // {
        //     return get_device(UartDeviceId::HAL_UART8);
        // }        

        // 妫€鏌ユ寚瀹歎ART璁惧鏄惁瀛樺湪
        virtual bool has_device(UartDeviceId id) const = 0;
};

//鑾峰彇UART鎬荤嚎鍗曚緥瀹炰緥
IUartBus &get_uart_bus_instance();

/********************* IMPL_UART_DEVICE *********************/

//UART纭欢璁惧瀹炵幇绫�
class UartDevice : public IUartDevice
{
    public:
        //鏋勯€犲嚱鏁帮紝鍒濆鍖朥ART璁惧
        explicit UartDevice(const UartDevice &) = delete;

        //鏋愭瀯鍑芥暟
        ~UartDevice() override = default;

        //绂佹鎷疯礉缁撴瀯鍜岃祴鍊兼搷浣�
        UartDevice(const UartDevice &) = delete;
        UartDevice &operator=(const UartDevice &) = delete;

        //瀹炵幇IUartDevice鎺ュ彛
        void init() override;
        void start() override;
        bool transmit(const Data &data) override;
        bool receive(Data &data) override;
        bool transmit_byte(uint8_t byte) override;
        bool receive_byte(uint8_t &byte) override;
        bool transmit_dma(const Data &data) override;
        bool receive_dma(Data &data) override;


        UART_HandleTypeDef *get_handle() const override;

    private:
        UART_HandleTypeDef *handle_;
        //鎺ユ敹鐘舵€佹爣蹇�
        bool is_receiving_;
        //DMA浼犺緭鐘舵€�
        bool is_dma_tx_ongoing_;        
        bool is_dma_rx_ongoing_;
        //绌洪棽涓柇鐘舵€�
        bool is_idle_enabled_;
};

//UART鎬荤嚎绠＄悊瀹炵幇绫�
class UartBus : public IUartBus
{
    public:
        //鑾峰彇鍗曚緥瀹炰緥
        static UartBus &instance();

        //鏋愭瀯鍑芥暟
        ~UartBus() override = default;

        //瀹炵幇IUartBus鎺ュ彛
        IUartDevice &get_device(UartDeviceId id) override;
        bool has_device(UartDeviceId id) const override;

        //鍒濆鍖朥ART鎬荤嚎
        void init();

    private:
        //绉佹湁鏋勯€犲嚱鏁帮紙鍗曚緥妯″紡锛�
        UartBus();

        //娉ㄥ唽涓€涓猆ART璁惧
        void register_device(UartDeviceId id, UartDevice *device);

        //鏄惁宸插垵濮嬪寲鏍囧織
        bool initalized_ = false;

        //绂佹鎷疯礉鏋勯€犲拰璧嬪€兼搷浣�
        UartBus(const UartBus &) = delete;
        UartBus &operator = (const UartBus &) = delete;

        //浣跨敤鎸囬拡鏁扮粍浠ｆ浛鍥哄畾鎴愬憳鍙橀噺
        UartDevice *devices_[(size_t)UartDeviceId::MAX_DEVICES] = {nullptr};

        //瀹為檯璁惧瀹炰緥
        UartDevice uart6_;

};
}

#endif
