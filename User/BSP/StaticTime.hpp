#pragma once
#include "../BSP/stdxxx.hpp"
#include "stm32f4xx_hal.h"

class RM_StaticTime
{
  public:
    uint32_t lastTime;                                                      // ��һʱ��
    void UpLastTime();                                                      // ������һʱ��
    bool ISOne(uint32_t targetTime);                                        // �жϵ����ź�
    bool ISGL(uint32_t targetTime, uint8_t percentage = 50 /*�ٷֱ�ռ��*/); // �ж������ź�
    bool ISDir(uint32_t dirTime);                                           // ��ʱ������
    bool ISFromOne(uint64_t nowTime, uint64_t targetTime);                  // �Զ����жϵ����ź�
    bool ISFromGL(uint64_t nowTime, uint64_t targetTime, uint8_t percentage = 50 /*�ٷֱ�ռ��*/); // �Զ����ж������ź�
};

inline void RM_StaticTime::UpLastTime()
{
    this->lastTime = HAL_GetTick();
}

inline bool RM_StaticTime::ISDir(uint32_t dirTime)
{
    if (HAL_GetTick() - this->lastTime >= dirTime)
        return true;

    return false;
}
