#include "Init.hpp"
#include "../BSP/CAN/Bsp_Can.hpp"

bool InitFlag = false;
void Init()
{

    CAN::BSP::Can_Init();


    InitFlag = true;
}
