/**
 * @file variables.cpp
 * @author XMX
 * @brief 全局变量创建
 * @version 1.0
 * @date 2024-08-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "variables.hpp"

PID pid_vel_204(8, 0, 0, 0, 16384); // 驱动轮速度环
