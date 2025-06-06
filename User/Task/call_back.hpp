#pragma once
#include "can.h"
#include "usart.h"

#ifdef Call_Back
extern "C"
{
#endif // Call_Back
    void Init();
    void InWhile(void);
#ifdef Call_Back
}
#endif // Call_Back
