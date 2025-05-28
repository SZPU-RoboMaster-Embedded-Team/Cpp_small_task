#pragma once
#include "can.h"
#include "usart.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
    extern uint8_t rx_buf[8];
    void Vofa_Motor_Control(void);

#ifdef __cplusplus
}
#endif // __cplusplus

