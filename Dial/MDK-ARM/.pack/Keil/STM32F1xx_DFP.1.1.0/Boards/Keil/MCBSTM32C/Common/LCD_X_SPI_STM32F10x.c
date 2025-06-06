/*********************************************************************
*                SEGGER Microcontroller GmbH & Co. KG                *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (c) 1996 - 2013  SEGGER Microcontroller GmbH & Co. KG       *
*                                                                    *
*        Internet: www.segger.com    Support:  support@segger.com    *
*                                                                    *
**********************************************************************

** emWin V5.22 - Graphical user interface for embedded applications **
All  Intellectual Property rights  in the Software belongs to  SEGGER.
emWin is protected by  international copyright laws.  Knowledge of the
source code may not be used to write a similar product.  This file may
only be used in accordance with the following terms:

The software has been licensed to  ARM LIMITED whose registered office
is situated at  110 Fulbourn Road,  Cambridge CB1 9NJ,  England solely
for  the  purposes  of  creating  libraries  for  ARM7, ARM9, Cortex-M
series,  and   Cortex-R4   processor-based  devices,  sublicensed  and
distributed as part of the  MDK-ARM  Professional  under the terms and
conditions  of  the   End  User  License  supplied  with  the  MDK-ARM
Professional. 
Full source code is available at: www.segger.com

We appreciate your understanding and fairness.
----------------------------------------------------------------------
File        : LCD_X_SPI_STM32F10x.c
Purpose     : Port routines for STM32F10x SPI Interface
----------------------------------------------------------------------
*/

#include "GUI.h"

/*********************************************************************
*
*       Hardware configuration
*
**********************************************************************
*/

#include <stm32f10x.h>
#include "GPIO_STM32F10x.h"
#include "Driver_SPI.h"

/* SPI Driver */
extern ARM_DRIVER_SPI Driver_SPI3;
#define ptrSPI      (&Driver_SPI3)

/*********************************************************************
*
*       Exported code
*
*********************************************************************
*/
/*********************************************************************
*
*       LCD_X_Init
*
* Purpose:
*   This routine should be called from your application program
*   to set port pins to their initial values
*/
void LCD_X_Init(void) {

  /* Initialize and configure SPI */
  ptrSPI->Initialize(NULL);
  ptrSPI->PowerControl(ARM_POWER_FULL);
  ptrSPI->Configure(ARM_SPI_CPOL1_CPHA1, ARM_SPI_MSB_LSB);
  ptrSPI->BusSpeed(18000000);

  /* LCD Backlight (PA.8) */
  GPIO_PortClock   (GPIOB, true);
  GPIO_PinConfigure(GPIOB, 0, GPIO_OUT_PUSH_PULL, GPIO_MODE_OUT50MHZ);
  GPIO_PinWrite    (GPIOB, 0, 1);       /* Turn backlight on */
}

/*********************************************************************
*
*       LCD_X_ClrCS
*
* Purpose:
*   Sets the chip select pin to low
*/
void LCD_X_ClrCS(void) {
  ptrSPI->SlaveSelect(ARM_SPI_SS_ACTIVE);
}

/*********************************************************************
*
*       LCD_X_SetCS
*
* Purpose:
*   Sets the chip select pin to high
*/
void LCD_X_SetCS(void) {
  ptrSPI->SlaveSelect(ARM_SPI_SS_INACTIVE);
}

/*********************************************************************
*
*       LCD_X_WriteM
*
* Purpose:
*   Writes multiple bytes to controller
*/
void LCD_X_WriteM(U8 * pData, int NumBytes) {
//ptrSPI->SendData(pData, NumBytes);
  while (NumBytes--) {
    ptrSPI->TransferByte(*pData++);
  }
}

/*********************************************************************
*
*       LCD_X_ReadM
*
* Purpose:
*   Reads multiple bytes from the controller
*/
void LCD_X_ReadM(U8 * pData, int NumBytes) {
//ptrSPI->ReceiveData(pData, NumBytes, 0);
  while (NumBytes--) {
    *pData++ = ptrSPI->TransferByte(0);
  }
}

/*************************** End of file ****************************/
