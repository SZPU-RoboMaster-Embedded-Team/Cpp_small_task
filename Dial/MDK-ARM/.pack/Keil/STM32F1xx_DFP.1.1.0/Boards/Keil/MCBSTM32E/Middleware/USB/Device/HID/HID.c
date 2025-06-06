/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::USB:Device
 * Copyright (c) 2004-2013 ARM Germany GmbH. All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    HID.c
 * Purpose: USB Device Human Interface Device example program
 *----------------------------------------------------------------------------*/

#include "cmsis_os.h"
#include "rl_usb.h"
#include "GLCD.h"
#include "Joystick.h"
#include "LED.h"

int main (void) {
  uint8_t but, but_last;

  but_last = 0;
  
  LED_Initialize     ();
  Joystick_Initialize();

  GLCD_Initialize    ();
  GLCD_Clear         (Blue);
  GLCD_SetBackColor  (Blue);
  GLCD_SetTextColor  (White);
  GLCD_DisplayString (0, 0, 1, "    USB Device      ");
  GLCD_DisplayString (1, 0, 1, "    HID Class       ");
  GLCD_DisplayString (2, 0, 1, "    HID Example     ");
  GLCD_DisplayString (4, 0, 1, "    USB: HID0       ");
  GLCD_DisplayString (8, 0, 1, "  Keil Tools by ARM ");
  GLCD_DisplayString (9, 0, 1, "    www.keil.com    ");

  USBD_Initialize    (0);               /* USB Device 0 Initialization        */
  USBD_Connect       (0);               /* USB Device 0 Connect               */

  while (1) {                           /* Loop forever                       */
    but = (uint8_t)(Joystick_GetButtons ());
    if (but != but_last) {
      but_last = but;
      USBD_HID_GetReportTrigger(0, 0, &but, 1);
    }

    osDelay(100);                       /* 100 ms delay for sampling buttons  */
  }
}
