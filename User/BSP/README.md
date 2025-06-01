# BSP层 (板级支持包)

BSP层负责提供特定板子的硬件配置和初始化，是连接HAL层和应用层的桥梁。

## 目录结构

- `UART_BSP/`: UART设备的板级实现
  - `impl/`: 具体实现文件
  - `uart_bsp.hpp`: BSP层接口定义
  - `uart_bsp.cpp`: BSP层实现

- `CAN_BSP/`: CAN设备的板级实现
  - `impl/`: 具体实现文件
  - `can_bsp.hpp`: BSP层接口定义
  - `can_bsp.cpp`: BSP层实现

## 职责范围

BSP层主要负责：

1. 硬件引脚配置
2. 时钟设置
3. 中断优先级配置
4. 板级设备初始化顺序
5. 特定板子的资源管理
6. 板载外设的抽象接口

## 使用方法

应用层应该通过BSP层提供的接口访问硬件，而不是直接调用HAL层。

示例：
```cpp
// 使用串口发送数据
Uart_BSP::uart1.sendData(data, length);


## 移植指南

当需要将代码移植到新的硬件板子时，只需要修改BSP层的具体实现，而不需要修改HAL层和应用层代码。 