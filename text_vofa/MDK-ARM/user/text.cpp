#include <bsp_can.hpp>
#include <FreeRTOS.h>
#include <DJI_motor.hpp>
#include <Control_principle.hpp>
#include <hal_can.hpp>
#include <hal_Serial_port.hpp>
#include <VOFA.hpp>

float M2006_1;
CONTROL::PID::PIDController  M2006_1_PID(1.0, 0.0, 0.0, 16384.0, 2000.0, 500.0);

void bsp_can(void const * argument)
{
    while(1)
    {
        Motorcontrol();
        osDelay(1);
    }
}

void vofa_uart(void const * argument)
{
    while(1)
    {   
        UART::VOFA::vofa_send(M2006_1, Motor2006.getVelocityRads, 0, 0, 0, 0);
        osDelay(1);
    }
}

void Motorcontrol()
{
    if(UART::VOFA::VOFAData::is_open  == 1)
    {
        M2006_1 = CONTROL::PID::PIDController::calculate(Motor2006.getVelocityRads, UART::VOFA::VOFAData::speed);
    }
    else if(UART::VOFA::VOFAData::is_open  == 0)
    {
        M2006_1 = 0;
    }
    Motor2006.setCAN(M2006_1, 1);
    Motor2006.sendCAN();
}