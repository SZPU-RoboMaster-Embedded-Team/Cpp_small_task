/* ----------------------------------------------------------------------
 * Copyright (C) 2013 ARM Limited. All rights reserved.
 *  
 * $Date:        12. September 2013
 * $Revision:    V1.03
 *  
 * Project:      OTG Full/Low-Speed Common Driver for ST STM32F10x
 * Configured:   via RTE_Device.h configuration file 
 * -------------------------------------------------------------------- */

/* History:
 *  Version 1.03
 *    Based on API V1.10 (namespace prefix ARM_ added)
 *  Version 1.00
 *    Initial release
 */ 

#include <stdint.h>
#include <string.h>
#include "cmsis_os.h"
#include "stm32f10x.h"

#include "GPIO_STM32F10x.h"
#include "OTG_STM32F10x_cl.h"

#include "Driver_USBH.h"
#include "Driver_USBD.h"

#include "RTE_Device.h"
#include "RTE_Components.h"

#if (RTE_USB_OTG_FS_HOST)
extern void USBH_FS_IRQ (uint32_t gintsts);
#endif
#if (RTE_USB_OTG_FS_DEV)
extern void USBD_FS_IRQ (uint32_t gintsts);
#endif

#if (RTE_USB_OTG_FS_HOST)
static bool    pins_configured_mask = 0;
#endif
       uint8_t otg_fs_role          = ARM_USB_ROLE_NONE;



/* Common IRQ Routine *********************************************************/

/**
  \fn          void OTG_FS_IRQHandler (void)
  \brief       USB Interrupt Routine (IRQ).
*/
void OTG_FS_IRQHandler (void) {
  uint32_t gintsts;

  gintsts = OTG_FS->GINTSTS & OTG_FS->GINTMSK;

  switch (otg_fs_role) {
    case ARM_USB_ROLE_HOST:
#if (RTE_USB_OTG_FS_HOST)
      USBH_FS_IRQ (gintsts);
#endif
      break;
    case ARM_USB_ROLE_DEVICE:
#if (RTE_USB_OTG_FS_DEV)
      USBD_FS_IRQ (gintsts);
#endif
      break;
    case ARM_USB_ROLE_NONE:
      break;
  }
}


/* Auxiliary Functions ********************************************************/

/* Pin Functions ---------------------*/

/**
  \fn          bool OTG_FS_PinsConfigure (uint8_t pins_mask)
  \brief       Configure single or multiple USB Pin(s).
  \param[in]   Mask of pins to be configured (possible masking values:
               USB_PIN_DP, USB_PIN_DM, USB_PIN_VBUS, USB_PIN_OC, USB_PIN_ID)
  \return      true = success, false = fail
*/
bool OTG_FS_PinsConfigure (uint8_t pins_mask) {

#if (RTE_USB_OTG_FS)
#if (RTE_USB_OTG_FS_HOST)
#if (!RTE_OTG_FS_VBUS_PIN)
#error   Configure VBUS Power On/Off pin for USB OTG Full-speed in RTE_Device.h!
#endif

  if ((pins_configured_mask ^ pins_mask) & ARM_USB_PIN_VBUS) {
    GPIO_PortClock        (RTE_OTG_FS_VBUS_PORT, true);
    if (!GPIO_PinConfigure(RTE_OTG_FS_VBUS_PORT, RTE_OTG_FS_VBUS_BIT, GPIO_OUT_OPENDRAIN, GPIO_MODE_OUT10MHZ)) return ARM_USBH_ERROR;
    pins_configured_mask |= ARM_USB_PIN_VBUS;
  }
  if ((pins_configured_mask ^ pins_mask) & ARM_USB_PIN_OC) {
    GPIO_PortClock        (RTE_OTG_FS_OC_PORT, true);
    if (!GPIO_PinConfigure(RTE_OTG_FS_OC_PORT, RTE_OTG_FS_OC_BIT, GPIO_IN_FLOATING, GPIO_MODE_INPUT)) return ARM_USBH_ERROR;
    pins_configured_mask |= ARM_USB_PIN_OC;
  }

#endif
#else
#error   Enable USB OTG in RTE_Device.h!
#endif


  return true;
}

/**
  \fn          bool OTG_FS_PinsUnconfigure (uint8_t pins_mask)
  \brief       Unconfigure to reset settings single or multiple USB Pin(s).
  \param[in]   Mask of pins to be unconfigured (possible masking values:
               USB_PIN_DP, USB_PIN_DM, USB_PIN_VBUS, USB_PIN_OC, USB_PIN_ID)
  \return      true = success, false = fail
*/
bool OTG_FS_PinsUnconfigure (uint8_t pins_mask) {

#if (RTE_USB_OTG_FS && RTE_USB_OTG_FS_HOST)

  if ((pins_configured_mask ^ pins_mask) & ARM_USB_PIN_VBUS) {
    GPIO_PortClock        (RTE_OTG_FS_VBUS_PORT, true);
    if (!GPIO_PinConfigure(RTE_OTG_FS_VBUS_PORT, RTE_OTG_FS_VBUS_BIT, GPIO_IN_FLOATING, GPIO_MODE_INPUT)) return ARM_USBH_ERROR;
    pins_configured_mask &= ~ARM_USB_PIN_VBUS;
  }
  if ((pins_configured_mask ^ pins_mask) & ARM_USB_PIN_OC) {
    GPIO_PortClock        (RTE_OTG_FS_OC_PORT, true);
    if (!GPIO_PinConfigure(RTE_OTG_FS_OC_PORT, RTE_OTG_FS_OC_BIT, GPIO_IN_FLOATING, GPIO_MODE_INPUT)) return ARM_USBH_ERROR;
    pins_configured_mask &= ~ARM_USB_PIN_OC;
  }

#endif

  return true;
}

/**
  \fn          bool OTG_FS_PinVbusOnOff (bool state)
  \brief       Drive VBUS Pin On/Off.
  \param[in]   state    State On/Off (true = On, false = Off)
  \return      true = success, false = fail
*/

bool OTG_FS_PinVbusOnOff (bool state) {

#if (RTE_OTG_FS_VBUS_PIN)
  GPIO_PinWrite (RTE_OTG_FS_VBUS_PORT, RTE_OTG_FS_VBUS_BIT, state == RTE_OTG_FS_VBUS_ACTIVE);
  return true;
#else
  return false;
#endif
}
