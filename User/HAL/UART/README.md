# UART驱动接口说明

本UART驱动封装了STM32 HAL库的UART功能，提供了更加易用、封装性好的现代C++接口。代码不依赖STL（除错误处理），适合嵌入式系统使用。接口和实现分离，支持更好的可扩展性和可测试性。设计遵循开闭原则，可以轻松添加新的UART设备而无需修改现有接口。

## 核心特性

- 面向对象设计，使用类和命名空间组织代码
- 接口和实现分离，遵循依赖倒置原则
- 设计符合开闭原则，可扩展而无需修改接口
- 单例模式管理UART总线实例（懒汉模式，自动初始化）
- 支持中断和DMA收发模式
- 支持UART空闲中断检测（用于不定长数据接收）
- 统一的收发接口
- 单字节快速收发API
- 错误处理和状态返回
- 提供可读性强的接口

## 文件结构

### 目录组织
- `uart_hal.hpp`: 主头文件，包含所有接口
- `README.md`: 说明文档
- `interface/`: 接口目录
  - `uart_device.hpp`: UART设备接口定义
  - `uart_bus.hpp`: UART总线接口定义
  - `uart_bus.cpp`: UART总线接口实现
- `impl/`: 实现目录
  - `uart_device_impl.hpp`: UART设备实现类定义
  - `uart_device_impl.cpp`: UART设备实现类实现
  - `uart_bus_impl.hpp`: UART总线实现类定义
  - `uart_bus_impl.cpp`: UART总线实现类实现

### 接口与实现分离
这种目录结构将接口与实现明确分离，带来以下好处：
1. 用户代码只需包含 `uart_hal.hpp` 即可使用所有功能
2. 实现细节被隐藏在 `impl` 目录中，用户不需要关注
3. 便于替换具体实现而不影响用户代码
4. 便于理解代码结构和职责划分

## 使用方法

### 初始化UART总线

采用懒汉模式，在第一次获取实例时自动初始化：

```cpp
// 获取实例时自动初始化UART总线
HAL::UART::get_uart_bus_instance();
```

### 发送数据

```cpp
// 获取UART设备
auto& uart1 = HAL::UART::get_uart_bus_instance().get_uart1();

// 创建数据结构
uint8_t buffer[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
HAL::UART::Data tx_data{buffer, 10};

// 发送数据（中断模式）
uart1.transmit(tx_data);

// 发送数据（DMA模式）
uart1.transmit_dma(tx_data);

// 发送单个字节
uart1.transmit_byte(0x55);
```

### 接收数据

```cpp
// 创建接收缓冲区
uint8_t rx_buffer[20];
HAL::UART::Data rx_data{rx_buffer, 20};

// 开始接收（中断模式）
uart1.receive(rx_data);

// 开始接收（DMA模式）
uart1.receive_dma(rx_data);

// 接收单个字节
uint8_t byte;
if (uart1.receive_byte(byte)) {
    // 处理接收到的字节
}
```

### 使用空闲中断接收不定长数据

空闲中断特别适用于接收不定长数据，当一帧数据传输后，线路上检测到空闲状态就会触发中断：

```cpp
// 准备大容量接收缓冲区
uint8_t buffer[256] = {0};
HAL::UART::Data rx_data{buffer, 256};

// 启动DMA连续接收并使能空闲中断
uart1.receive_dma_idle(rx_data);
```

**重要**: 需要在`stm32f4xx_it.c`中修改UART中断处理函数，添加空闲中断检测：

```c
// 在stm32f4xx_it.c中修改
void USART1_IRQHandler(void)
{
    // 使用C语法访问C++对象
    extern UART_HandleTypeDef huart1;
    extern void UART_IdleCallback(UART_HandleTypeDef *huart);
    
    // 检测是否为空闲中断
    if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) != RESET)
    {
        // 清除空闲中断标志
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);
        
        // 调用空闲中断处理函数
        UART_IdleCallback(&huart1);
    }
    
    // 处理其他中断
    HAL_UART_IRQHandler(&huart1);
}
```

然后，需要实现空闲中断处理函数：

```cpp
// 在合适的C++文件中实现
extern "C" {
    void UART_IdleCallback(UART_HandleTypeDef *huart)
    {
        if(huart->Instance == USART1)
        {
            auto& uart1 = HAL::UART::get_uart_bus_instance().get_uart1();
            
            // 停止当前传输
            uart1.abort_dma();
            
            // 计算接收到的数据长度
            uint16_t len = 256 - __HAL_DMA_GET_COUNTER(huart->hdmarx);
            
            // 此时可以处理接收到的数据
            // process_data(buffer, len);
            
            // 重新启动接收
            HAL::UART::Data new_data{buffer, 256};
            uart1.receive_dma_idle(new_data);
        }
    }
}
```

### 多UART设备管理

```cpp
auto& uart_bus = HAL::UART::get_uart_bus_instance();

// 获取特定UART设备
auto& uart1 = uart_bus.get_uart1();
auto& uart3 = uart_bus.get_uart3();
auto& uart6 = uart_bus.get_uart6();

// 通过ID获取设备
auto& uart = uart_bus.get_device(HAL::UART::UartDeviceId::HAL_Uart1);

// 检查设备是否存在
if (uart_bus.has_device(HAL::UART::UartDeviceId::HAL_Uart6)) {
    // 使用UART6
}
```

### 中断回调函数

在应用层可以重写以下回调函数来处理UART事件：

```cpp
// 在适当的.c/.cpp文件中实现这些函数
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    // 发送完成处理
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    // 接收完成处理
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    // 错误处理
}
``` 