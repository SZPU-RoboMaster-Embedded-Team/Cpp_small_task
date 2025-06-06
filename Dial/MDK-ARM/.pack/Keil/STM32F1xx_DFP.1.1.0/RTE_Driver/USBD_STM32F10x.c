/* ----------------------------------------------------------------------
 * Copyright (C) 2013 ARM Limited. All rights reserved.
 *  
 * $Date:        12. September 2013
 * $Revision:    V1.00
 *  
 * Driver:       Driver_USBD0
 * Configured:   via RTE_Device.h configuration file 
 * Project:      USB Full/Low-Speed Device Driver for ST STM32F10x
 * ---------------------------------------------------------------------- 
 * Use the following configuration settings in the middleware component
 * to connect to this driver.
 * 
 *   Configuration Setting                Value
 *   ---------------------                -----
 *   Connect to hardware via Driver_USBD# = 0
 * -------------------------------------------------------------------- */

/* History:
 *  Version 1.00
 *    Initial release
 */ 

#include <stdint.h>
#include <string.h>
#include "cmsis_os.h"
#include "stm32f10x.h"

#include "GPIO_STM32F10x.h"
#include "USBD_STM32F10x.h"


#include "Driver_USBD.h"

#include "RTE_Device.h"
#include "RTE_Components.h"


/* GLOBAL VARIABLES */
#define EP_BUF_ADDR (sizeof(EP_BUF_DSCR)*9)                 // Endpoint Buf Adr

EP_BUF_DSCR *pBUF_DSCR = (EP_BUF_DSCR *)USB_PMA_ADDR;       // Ptr to EP Buf Desc

uint16_t FreeBufAddr;                                       // Endpoint Free Buffer Address

uint16_t InEpMaxPacketSz[8];
uint16_t OutEpMaxPacketSz[8];


/* USBD Driver ****************************************************************/

#define ARM_USBD_DRV_VERSION ARM_DRV_VERSION_MAJOR_MINOR(1,00) /* USBD driver version */

/* Driver Version */
static const ARM_DRV_VERSION usbd_driver_version = { ARM_USBD_API_VERSION, ARM_USBD_DRV_VERSION };

/* Driver Capabilities */
static const ARM_USBD_CAPABILITIES usbd_driver_capabilities = {
  false,  /* event_power_on      */
  false,  /* event_power_off     */
  true,   /* event_connect       */
  false,  /* event_disconnect    */
  true,   /* event_reset         */
  true,   /* event_high_speed    */
  true,   /* event_suspend       */
  true,   /* event_resume        */
  false,  /* event_remote_wakeup */
  false   /* reserved            */
};

/* Static Variables */
static ARM_USBD_SignalDeviceEvent_t   cbDeviceEvent;
static ARM_USBD_SignalEndpointEvent_t cbEndpointEvent;

static ARM_USBD_STATE UsbdState = {0, 0, 0, 0};

/* LOCAL FUNCTIONS */

/**
  \fn          static void EP_Reset (uint8_t ep_addr)
  \brief       Called to reset Endpoint configuration
*/
static void EP_Reset (uint8_t ep_addr) {
  uint32_t num, val;

  num = ep_addr & ARM_USB_ENDPOINT_NUMBER_MASK;
  val = EPxREG(num);
  if (ep_addr & ARM_USB_ENDPOINT_DIRECTION_MASK) {  // IN Endpoint
    EPxREG(num) = val & (EP_MASK | EP_DTOG_TX);
  } else {                                          // OUT Endpoint
    EPxREG(num) = val & (EP_MASK | EP_DTOG_RX);
  }
}

/**
  \fn          static void EP_Status (uint8_t ep_addr, uint32_t status)
  \brief       Called to retrieve Endpoint status
*/
void EP_Status (uint8_t ep_addr, uint32_t stat) {
  uint32_t num, val;

  num = ep_addr & ARM_USB_ENDPOINT_NUMBER_MASK;
  val = EPxREG(num);
  if (ep_addr & ARM_USB_ENDPOINT_DIRECTION_MASK) {  // IN Endpoint
    EPxREG(num) = (val ^ (stat & EP_STAT_TX)) & (EP_MASK | EP_STAT_TX);
  } else {                                          // OUT Endpoint
    EPxREG(num) = (val ^ (stat & EP_STAT_RX)) & (EP_MASK | EP_STAT_RX);
  }
}


/**
  \fn          static void USBD_Reset (uint8_t ep_addr)
  \brief       Called after usbd reset interrupt to reset configuration
*/
static void USBD_Reset (void) {
  uint32_t val;

  ISTR = 0;                             // Clear Interrupt Status

  CNTR = CNTR_CTRM | CNTR_RESETM | CNTR_SUSPM | CNTR_WKUPM | CNTR_ERRM | CNTR_PMAOVRM;

  FreeBufAddr = EP_BUF_ADDR;

  BTABLE = 0x00;                        // set BTABLE Address

  val = OutEpMaxPacketSz[0];


  /* Setup Control Endpoint 0 */
  pBUF_DSCR->ADDR_TX = FreeBufAddr;
  FreeBufAddr += val;
  pBUF_DSCR->ADDR_RX = FreeBufAddr;
  FreeBufAddr += val;
  if (val > 62) {
    val = (val + 31) & ~31;
    pBUF_DSCR->COUNT_RX = ((val << 5) - 1) | 0x8000;
  } else {
    val = (val + 1)  & ~1;
    pBUF_DSCR->COUNT_RX =   val << 9;
  }

  EPxREG(0) = EP_CONTROL | EP_RX_VALID;

  DADDR = DADDR_EF | 0;                 // Enable USB Default Address

}

/* FUNCTION PROTOTYPES */
static ARM_USBD_STATUS USBD_HW_PowerControl   (ARM_POWER_STATE state);
static uint16_t        USBD_HW_GetFrameNumber (void);


/* USB DEVICE DRIVER FUNCTIONS */

/**
  \fn          ARM_DRV_VERSION USBD_GetVersion (void)
  \brief       Get driver version.
  \return      \ref ARM_DRV_VERSION
*/
static ARM_DRV_VERSION USBD_HW_GetVersion (void) { return usbd_driver_version; }

/**
  \fn          ARM_USBD_CAPABILITIES USBD_GetCapabilities (void)
  \brief       Get driver capabilities.
  \return      \ref ARM_USBD_CAPABILITIES
*/
static ARM_USBD_CAPABILITIES USBD_HW_GetCapabilities (void) { return usbd_driver_capabilities; };

/**
  \fn          ARM_USBD_STATUS USBD_Initialize (ARM_USBD_SignalDeviceEvent_t   cb_device_event,
                                                ARM_USBD_SignalEndpointEvent_t cb_endpoint_event)
  \brief       Initialize USB Device Interface.
  \param[in]   cb_device_event    Pointer to \ref USBD_SignalDeviceEvent
  \param[in]   cb_endpoint_event  Pointer to \ref USBD_SignalEndpointEvent
  \return      \ref USBD_STATUS
*/
static ARM_USBD_STATUS USBD_HW_Initialize (ARM_USBD_SignalDeviceEvent_t   cb_device_event,
                                           ARM_USBD_SignalEndpointEvent_t cb_endpoint_event) {

#if (RTE_USB_DEVICE)
  cbDeviceEvent   = cb_device_event;
  cbEndpointEvent = cb_endpoint_event;


#if (RTE_USB_DEVICE_CON_PIN)
  // Configure CON pin (controls pull-up on D+ line)
  GPIO_PortClock (RTE_USB_DEVICE_CON_PORT, true);
  GPIO_PinWrite (RTE_USB_DEVICE_CON_PORT, RTE_USB_DEVICE_CON_BIT, !RTE_OTG_FS_VBUS_ACTIVE);
  if (!GPIO_PinConfigure(RTE_USB_DEVICE_CON_PORT, RTE_USB_DEVICE_CON_BIT, GPIO_OUT_OPENDRAIN, GPIO_MODE_OUT10MHZ)) return ARM_USBD_ERROR;
#endif

  RCC->APB1ENR   |=  RCC_APB1ENR_USBEN;         // Enable USB Device clock
  RCC->APB1RSTR  |=  RCC_APB1RSTR_USBRST;       // Reset USB Device
  osDelay(1);                                   // Wait 1 ms
  RCC->APB1RSTR  &= ~RCC_APB1RSTR_USBRST;       // Reset USB Device
#endif

  return ARM_USBD_OK;
}

/**
  \fn          ARM_USBD_STATUS USBD_Uninitialize (void)
  \brief       De-initialize USB Device Interface.
  \return      \ref ARM_USBD_STATUS
*/
static ARM_USBD_STATUS USBD_HW_Uninitialize (void) {

#if (RTE_USB_DEVICE)
  USBD_HW_PowerControl (ARM_POWER_OFF);
  RCC->APB1RSTR  |=  RCC_APB1RSTR_USBRST;       // Reset USB Device
  osDelay(1);                                   // Wait 1 ms
  RCC->APB1RSTR  &= ~RCC_APB1RSTR_USBRST;       // Reset USB Device
  RCC->APB1ENR   &= ~RCC_APB1ENR_USBEN;         // Disable USB Device clock
#endif

#if (RTE_USB_DEVICE_CON_PIN)
  // Unconfigure CON pin (controls pull-up on D+ line)
  GPIO_PortClock (RTE_USB_DEVICE_CON_PORT, true);
  GPIO_PinWrite (RTE_USB_DEVICE_CON_PORT, RTE_USB_DEVICE_CON_BIT, !RTE_OTG_FS_VBUS_ACTIVE);
  if (!GPIO_PinConfigure(RTE_USB_DEVICE_CON_PORT, RTE_USB_DEVICE_CON_BIT, GPIO_IN_FLOATING, GPIO_MODE_INPUT)) return ARM_USBD_ERROR;
#endif

  return ARM_USBD_OK;
}

/**
  \fn          ARM_USBD_STATUS USBD_HW_PowerControl (ARM_POWER_STATE state)
  \brief       Control USB Device Interface Power.
  \param[in]   state specifies Power state
  \return      \ref ARM_USBD_STATUS
*/
static ARM_USBD_STATUS USBD_HW_PowerControl (ARM_POWER_STATE state) {
  switch (state) {
    case ARM_POWER_OFF:
      NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);    // Disable USB Device interrupt
      
      RCC->APB1ENR   |=  RCC_APB1ENR_USBEN;     // Enable USB Device clock
      osDelay(1);                               // Wait 1 ms

      CNTR = CNTR_FRES | CNTR_PDWN;             // Switch off USB Device

      // Soft disconnect
      GPIO_PinWrite (RTE_USB_DEVICE_CON_PORT, RTE_USB_DEVICE_CON_BIT, !RTE_OTG_FS_VBUS_ACTIVE);

      RCC->APB1ENR   &= ~RCC_APB1ENR_USBEN;     // Disable USB Device clock

      break;

    case ARM_POWER_LOW:
      return ARM_USBD_ERROR;

    case ARM_POWER_FULL:
      RCC->APB1ENR   |=  RCC_APB1ENR_USBEN;     // Enable USB Device clock
      RCC->APB1RSTR  |=  RCC_APB1RSTR_USBRST;   // Reset USB Device
      osDelay(1);                               // Wait 1 ms
      RCC->APB1RSTR  &= ~RCC_APB1RSTR_USBRST;   // Reset USB Device

      NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);     // Enable USB Device interrupt
      USBD_Reset ();
      break;
  }
  return ARM_USBD_OK;
}

/**
  \fn          ARM_USBD_STATUS USBD_HW_DeviceConnect (void)
  \brief       Connect USB Device.
  \return      \ref ARM_USBD_STATUS
*/
static ARM_USBD_STATUS USBD_HW_DeviceConnect (void) {
  // Soft connect
  GPIO_PinWrite (RTE_USB_DEVICE_CON_PORT, RTE_USB_DEVICE_CON_BIT, RTE_OTG_FS_VBUS_ACTIVE);

  CNTR = CNTR_FRES;                             // Force USB Reset
  CNTR = 0;
  ISTR = 0;                                     // Clear Interrupt Status
  CNTR = CNTR_RESETM | CNTR_SUSPM | CNTR_WKUPM; // USB Interrupt Mask

  return ARM_USBD_OK;
}

/**
  \fn          ARM_USBD_STATUS USBD_HW_DeviceDisconnect (void)
  \brief       Disconnect USB Device.
  \return      \ref ARM_USBD_STATUS
*/
static ARM_USBD_STATUS USBD_HW_DeviceDisconnect (void) {

  CNTR = CNTR_FRES | CNTR_PDWN;                 // Switch Off USB Device

  // Soft disconnect
  GPIO_PinWrite (RTE_USB_DEVICE_CON_PORT, RTE_USB_DEVICE_CON_BIT, !RTE_OTG_FS_VBUS_ACTIVE);

  UsbdState.connected = false;
  UsbdState.active    = false;

  return ARM_USBD_OK;
}

/**
  \fn          ARM_USBD_STATE USBD_HW_DeviceGetState (void)
  \brief       Get current USB Device State.
  \return      \ref ARM_USBD_STATE
*/
static ARM_USBD_STATE USBD_HW_DeviceGetState (void) {
  return UsbdState;
}

/**
  \fn          ARM_USBD_STATUS USBD_HW_DeviceRemoteWakeup (void)
  \brief       Trigger USB Remote Wakeup.
  \return      \ref ARM_USBD_STATUS
*/
static ARM_USBD_STATUS USBD_HW_DeviceRemoteWakeup (void) {
  CNTR &= ~CNTR_FSUSP;                          // Clear Suspend
  return ARM_USBD_OK;
}

/**
  \fn          ARM_USBD_STATUS USBD_HW_DeviceSetAddress (uint8_t dev_addr,
                                                         ARM_USBD_SET_ADDRESS_STAGE stage)
  \brief       Set USB Device Address.
  \param[in]   dev_addr specifies Device Address
  \param[in]   stage specifies stage in which the function is called
  \return      \ref ARM_USBD_STATUS
*/
static ARM_USBD_STATUS USBD_HW_DeviceSetAddress (uint8_t dev_addr,
                                                 ARM_USBD_SET_ADDRESS_STAGE stage) {
  if (stage == ARM_USBD_SET_ADDRESS_STATUS) DADDR = DADDR_EF | dev_addr;

  return ARM_USBD_OK;
}

/**
  \fn          ARM_USBD_STATUS USBD_HW_DeviceConfigure (bool configure)
  \brief       Configure/unconfigure USB Device.
  \param[in]   configure specifies operation
                - \b false Unconfigure
                - \b true  Configure
  \return      \ref ARM_USBD_STATUS
*/
static ARM_USBD_STATUS USBD_HW_DeviceConfigure (bool configure) {
    if (configure == false) {
      FreeBufAddr  = EP_BUF_ADDR;
      FreeBufAddr += 2 * OutEpMaxPacketSz[0];   // reset Buffer address
  }
  return ARM_USBD_OK;
}

/**
  \fn          ARM_USBD_STATUS USBD_HW_EndpointConfigure (uint8_t ep_addr,
                                                          ARM_USB_ENDPOINT_TYPE ep_type,
                                                          uint16_t ep_max_packet_size)
  \brief       Configure USB Endpoint.
  \param[in]   ptr_epd specifies pointer to Endpoint descriptor
  \return      \ref ARM_USBD_STATUS
*/
static ARM_USBD_STATUS USBD_HW_EndpointConfigure (uint8_t ep_addr,
                                                  ARM_USB_ENDPOINT_TYPE ep_type,
                                                  uint16_t ep_max_packet_size) {

  uint32_t num, val;

  num = ep_addr & ARM_USB_ENDPOINT_NUMBER_MASK;
  val = ep_max_packet_size & ARM_USB_ENDPOINT_MAX_PACKET_SIZE_MASK;

  if (num) {
    if (ep_addr & ARM_USB_ENDPOINT_DIRECTION_MASK) {
      InEpMaxPacketSz[num] = val;
      (pBUF_DSCR + num)->ADDR_TX = FreeBufAddr;
      val = (val + 1) & ~1;
    } else {
      OutEpMaxPacketSz[num] = val;
      (pBUF_DSCR + num)->ADDR_RX = FreeBufAddr;
      if (val > 62) {
        val = (val + 31) & ~31;
        (pBUF_DSCR + num)->COUNT_RX = ((val << 5) - 1) | 0x8000;
      } else {
        val = (val + 1)  & ~1;
        (pBUF_DSCR + num)->COUNT_RX =   val << 9;
      }
    }
    FreeBufAddr += val;

    switch (ep_type) {
      case ARM_USB_ENDPOINT_CONTROL:
        val = EP_CONTROL;
        break;
      case ARM_USB_ENDPOINT_ISOCHRONOUS:
        val = EP_ISOCHRONOUS;
        break;
      case ARM_USB_ENDPOINT_BULK:
        val = EP_BULK;
        break;
      case ARM_USB_ENDPOINT_INTERRUPT:
        val = EP_INTERRUPT;
        break;
    }
    val |= num;
    EPxREG(num) = val;

    EP_Reset(ep_addr);
    EP_Status(ep_addr, EP_TX_NAK | EP_RX_VALID);// EP is able to receive

  } else {
    InEpMaxPacketSz[num] = OutEpMaxPacketSz[num] = val;
  }

  return ARM_USBD_OK;
}

/**
  \fn          ARM_USBD_STATUS USBD_HW_EndpointUnconfigure (uint8_t ep_addr)
  \brief       Unconfigure USB Endpoint.
  \param[in]   ep_addr specifies Endpoint Address
                ep_addr.0..3: Address
                ep_addr.7:    Direction
  \return      \ref ARM_USBD_STATUS
*/
static ARM_USBD_STATUS USBD_HW_EndpointUnconfigure (uint8_t ep_addr) {
  EP_Reset (ep_addr);
  EP_Status(ep_addr, EP_TX_DIS | EP_RX_DIS);
  
  return ARM_USBD_OK;
}

/**
  \fn          ARM_USBD_STATUS USBD_HW_EndpointStall (uint8_t ep_addr, bool stall)
  \brief       Set/Clear Stall for USB Endpoint.
  \param[in]   ep_addr specifies Endpoint Address
                ep_addr.0..3: Address
                ep_addr.7:    Direction
  \param[in]   stall specifies operation
                - \b false Clear
                - \b true Set
  \return      \ref ARM_USBD_STATUS
*/
static ARM_USBD_STATUS USBD_HW_EndpointStall (uint8_t ep_addr, bool stall) {
  if (stall == true) {
    EP_Status(ep_addr, EP_TX_STALL | EP_RX_STALL);
  } else {
    EP_Reset(ep_addr);                          // reset DTog Bits
    EP_Status(ep_addr, EP_TX_VALID | EP_RX_VALID);
  }
  return ARM_USBD_OK;
}

/**
\fn          ARM_USBD_STATUS USBD_HW_EndpointReadStart (uint8_t ep_addr, uint8_t *buf, uint32_t len)
\brief       Start USB Endpoint Read operation.
\param[in]   ep_addr specifies Endpoint Address
              ep_addr.0..3: Address
              ep_addr.7:    Direction
\param[out]  buf specifies buffer for data read from Endpoint
\param[in]   len specifies buffer length
\return      \ref ARM_USBD_STATUS
*/
static ARM_USBD_STATUS USBD_HW_EndpointReadStart (uint8_t ep_addr, uint8_t *buf, uint32_t len) {
  return ARM_USBD_OK;
}


/**
  \fn          int32_t USBD_HW_EndpointRead (uint8_t ep_addr, uint8_t *buf, uint32_t len)
  \brief       Read data from USB Endpoint.
  \param[in]   ep_addr specifies Endpoint Address
                ep_addr.0..3: Address
                ep_addr.7:    Direction
  \param[out]  buf specifies buffer for data read from Endpoint
  \param[in]   len specifies buffer length
  \return      number of data bytes read, error code if negative
*/
static int32_t USBD_HW_EndpointRead (uint8_t ep_addr, uint8_t *buf, uint32_t len) {
  uint32_t num, cnt, *pv, n;
  bool     flag;
  uint8_t  tmp_buf[2];

  num = ep_addr & ARM_USB_ENDPOINT_NUMBER_MASK;

  pv  = (uint32_t *)(USB_PMA_ADDR + 2*((pBUF_DSCR + num)->ADDR_RX));
  cnt = (pBUF_DSCR + num)->COUNT_RX & EP_COUNT_MASK;
  
  if (cnt % 2) flag = true;
  else         flag = false;
  
  for (n = 0; n < (cnt / 2); n++) {
    *((__packed uint16_t *)buf) = *pv++;
    buf += 2;
  }
  if (flag == true) {
    *((__packed uint16_t *)tmp_buf) = *pv++;
    *buf = *tmp_buf;
  }
  EP_Status(ep_addr, EP_RX_VALID);

  return (cnt);
}

/**
  \fn          int32_t USBD_HW_EndpointWrite (uint8_t ep_addr, const uint8_t *buf, uint32_t len)
  \brief       Write data to USB Endpoint.
  \param[in]   ep_addr specifies Endpoint Address
                ep_addr.0..3: Address
                ep_addr.7:    Direction
  \param[in]   buf specifies buffer with data to write to Endpoint
  \param[in]   len specifies buffer length
  \return      number of data bytes written, error code if negative
*/
static int32_t USBD_HW_EndpointWrite (uint8_t ep_addr, const uint8_t *buf, uint32_t len) {

  uint32_t num, *pv, n;
  uint16_t statusEP;

  num = ep_addr & ARM_USB_ENDPOINT_NUMBER_MASK;


  if (len > InEpMaxPacketSz[num])
    len = InEpMaxPacketSz[num];

  pv  = (uint32_t *)(USB_PMA_ADDR + 2*((pBUF_DSCR + num)->ADDR_TX));
  for (n = 0; n < (len + 1) / 2; n++) {
    *pv++ = *((__packed uint16_t *)buf);
    buf += 2;
  }
  (pBUF_DSCR + num)->COUNT_TX = len;

  statusEP = EPxREG(num);
  if ((statusEP & EP_STAT_TX) != EP_TX_STALL) {
    EP_Status(ep_addr, EP_TX_VALID);            // do not make EP valid if stalled
  }
  
 return (len);
}

/**
  \fn          ARM_USBD_STATUS USBD_HW_EndpointAbort (uint8_t ep_addr)
  \brief       Abort current USB Endpoint transfer.
  \param[in]   ep_addr specifies Endpoint Address
                ep_addr.0..3: Address
                ep_addr.7:    Direction
  \return      \ref ARM_USBD_STATUS
*/
static ARM_USBD_STATUS USBD_HW_EndpointAbort (uint8_t ep_addr) {

  return ARM_USBD_OK;
}

/**
  \fn          uint16_t USBD_HW_GetFrameNumber (void)
  \brief       Get current USB Frame Number.
  \return      Frame Number
*/
static uint16_t USBD_HW_GetFrameNumber (void) {
  return (FNR & FNR_FN);
}

/**
  \fn          void USB_LP_CAN1_RX0_IRQHandler (uint32_t gintsts)
  \brief       USB Device Interrupt Routine (IRQ).
*/
void USB_LP_CAN1_RX0_IRQHandler (uint32_t gintsts) {

  uint32_t istr, num, val;

  istr = ISTR;

// Reset interrupt
  if (istr & ISTR_RESET) {
    UsbdState.active    = false;
    UsbdState.connected = true;
    USBD_Reset();
    cbDeviceEvent(ARM_USBD_EVENT_RESET);
    ISTR = ~ISTR_RESET;
  }

// Suspend interrupt
  if (istr & ISTR_SUSP) {
    UsbdState.active = false;
    cbDeviceEvent(ARM_USBD_EVENT_SUSPEND);
    ISTR = ~ISTR_SUSP;
  }

// Resume interrupt
  if (istr & ISTR_WKUP) {
    UsbdState.active = true;
    cbDeviceEvent(ARM_USBD_EVENT_RESUME);
    ISTR = ~ISTR_WKUP;
  }

// PMA Over/underrun
  if (istr & ISTR_PMAOVR) {
    ISTR = ~ISTR_PMAOVR;
  }

// Error: No Answer, CRC Error, Bit Stuff Error, Frame Format Error
  if (istr & ISTR_ERR) {
    ISTR = ~ISTR_ERR;
  }

// Endpoint interrupts
  if ((istr = ISTR) & ISTR_CTR) {
//    ISTR = ~ISTR_CTR;

    num = istr & ISTR_EP_ID;

    val = EPxREG(num);
    if (val & EP_CTR_RX) {
      EPxREG(num) = val & ~EP_CTR_RX & EP_MASK;

// Setup Packet
      if (val & EP_SETUP) {
        cbEndpointEvent(num, ARM_USBD_EP_EVENT_SETUP);
      } else {
// OUT Packet
        cbEndpointEvent(num, ARM_USBD_EP_EVENT_OUT);
      }
    }

// IN Packet
    if (val & EP_CTR_TX) {
      EPxREG(num) = val & ~EP_CTR_TX & EP_MASK;
      cbEndpointEvent(num, ARM_USBD_EP_EVENT_IN);
    }
  }
}

ARM_DRIVER_USBD Driver_USBD0 = {
  USBD_HW_GetVersion,
  USBD_HW_GetCapabilities,
  USBD_HW_Initialize,
  USBD_HW_Uninitialize,
  USBD_HW_PowerControl,
  USBD_HW_DeviceConnect,
  USBD_HW_DeviceDisconnect,
  USBD_HW_DeviceGetState,
  USBD_HW_DeviceRemoteWakeup,
  USBD_HW_DeviceSetAddress,
  USBD_HW_DeviceConfigure,
  USBD_HW_EndpointConfigure,
  USBD_HW_EndpointUnconfigure,
  USBD_HW_EndpointStall,
  USBD_HW_EndpointReadStart,
  USBD_HW_EndpointRead,
  USBD_HW_EndpointWrite,
  USBD_HW_EndpointAbort,
  USBD_HW_GetFrameNumber
};
