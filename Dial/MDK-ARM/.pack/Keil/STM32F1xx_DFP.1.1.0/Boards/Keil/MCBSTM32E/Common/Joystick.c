/*-----------------------------------------------------------------------------
 * Name:    JOY.c
 * Purpose: Low level joystick functions
 * Note(s):
 *-----------------------------------------------------------------------------
 * This file is part of the uVision/ARM development tools.
 * This software may only be used under the terms of a valid, current,
 * end user licence from KEIL for a compatible version of KEIL software
 * development tools. Nothing else gives you the right to use this software.
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 * Copyright (c) 2004-2013 KEIL - An ARM Company. All rights reserved.
 *----------------------------------------------------------------------------*/

#include "stm32f10x.h"                  /* STM32F10x Definitions              */
#include "Joystick.h"
#include "GPIO_STM32F10x.h"

typedef struct {
  GPIO_PIN_ID Pin;
  uint8_t     Direction;
} JOYSTICK_MAP;


const JOYSTICK_MAP Joystick[] = {
  {{ GPIOG,  14 }, JOY_LEFT},   // left  
  {{ GPIOG,  13 }, JOY_RIGHT},  // right
  {{ GPIOG,   7 }, JOY_CENTER}, // center / select  
  {{ GPIOG,  15 }, JOY_UP},     // up
  {{ GPIOD,   3 }, JOY_DOWN}    // down 
};

#define NUM_DIRECTIONS (sizeof(Joystick)/sizeof(JOYSTICK_MAP))

/*-----------------------------------------------------------------------------
 *  Joystick_Initialize:  Initialize joystick
 *----------------------------------------------------------------------------*/
void Joystick_Initialize(void) {
  uint32_t idx;
  
  /* Configure Pins */
  for (idx = 0; idx < NUM_DIRECTIONS; idx++) {
    GPIO_PortClock   (Joystick[idx].Pin.port, true);
    GPIO_PinConfigure(Joystick[idx].Pin.port, Joystick[idx].Pin.num,
                      GPIO_IN_FLOATING, 
                      GPIO_MODE_INPUT);
  }
}


/*-----------------------------------------------------------------------------
 *  Joystick_Uninitialize:  Uninitialize joystick
 *----------------------------------------------------------------------------*/
void Joystick_Uninitialize (void) {
  uint32_t idx;
  
  for (idx = 0; idx < NUM_DIRECTIONS; idx++) {
    GPIO_PinConfigure(Joystick[idx].Pin.port, Joystick[idx].Pin.num,
                      GPIO_OUT_PUSH_PULL, 
                      GPIO_MODE_OUT2MHZ);
  }
}


/*-----------------------------------------------------------------------------
 *  Joystick_GetButtons:  Get joystick direction keys state
 *
 * Parameters: (none)
 * Return:     joystick directions bitmask
 *----------------------------------------------------------------------------*/
uint32_t Joystick_GetButtons  (void) {
  uint32_t pd;
  uint32_t idx;
  uint32_t btn = 0;
  
  for (idx = 0; idx < NUM_DIRECTIONS; idx++) { 
    pd = GPIO_PortRead(Joystick[idx].Pin.port);
    if ( (pd & (1 << Joystick[idx].Pin.num)) == 0) {
      btn |= Joystick[idx].Direction;
    }
  }  
  return btn;
}
