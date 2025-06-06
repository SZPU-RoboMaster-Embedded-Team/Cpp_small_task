/* ----------------------------------------------------------------------
 * Copyright (C) 2013 ARM Limited. All rights reserved.
 *  
 * $Date:        12. September 2013
 * $Revision:    V1.01
 *  
 * Driver:       Driver_MCI0
 * Configured:   via RTE_Device.h configuration file 
 * Project:      MCI Driver for STMicroelectronics STM32F10x
 * ---------------------------------------------------------------------- 
 * Use the following configuration settings in the middleware component
 * to connect to this driver.
 * 
 *   Configuration Setting                 Value
 *   ---------------------                 -----
 *   Connect to hardware via Driver_MCI# = 0
 * -------------------------------------------------------------------- */

/* History:
 *  Version 1.01
 *    Based on API V1.10 (namespace prefix ARM_ added)
 *  Version 1.00
 *    Initial release
 */

#include "cmsis_os.h"
#include "STM32F10x.h"

#include "GPIO_STM32F10x.h"
#include "DMA_STM32F10x.h"

#include "Driver_MCI.h"

#include "RTE_Device.h"
#include "RTE_Components.h"

#define ARM_MCI_DRV_VERSION ARM_DRV_VERSION_MAJOR_MINOR(1,01)  /* driver version */


#if (defined(RTE_Drivers_MCI0) && !RTE_SDIO)
#error "SDIO not configured in RTE_Device.h!"
#endif

#if (!RTE_SDIO_DMA)
#error "SDIO requires DMA! Enable DMA in RTE_Device.h!"
#endif

#if ( (RTE_SDIO_DMA_NUMBER   != 2) || (RTE_SDIO_DMA_CHANNEL  != 4) || \
     ((RTE_SDIO_DMA_PRIORITY  < 0) || (RTE_SDIO_DMA_PRIORITY > 3)))
#error "SDIO DMA configuration in RTE_Device.h is invalid!"
#endif


#define MCI_CMD_SEND_TIMEOUT       10000 /* Command send timeout in us */
#define MCI_CMD_RESPONSE_TIMEOUT     100 /* Command response timeout in ms */
#define MCI_READ_TRANSFER_TIMEOUT    150 /* Read Transfer Timeout in ms */
#define MCI_WRITE_TRANSFER_TIMEOUT   300 /* Write Transfer Timeout in ms */


/* MCI messages */
#define MCI_MSG_ABORT           (1UL << 31)
#define MCI_MSG_DMA_DONE        (1UL << 30)
#define MCI_MSG_DMA_ERROR       (1UL << 29)

/* Interrupt clear Mask */
#define SDIO_ICR_BIT_MASK       SDIO_ICR_CCRCFAILC | \
                                SDIO_ICR_DCRCFAILC | \
                                SDIO_ICR_CTIMEOUTC | \
                                SDIO_ICR_DTIMEOUTC | \
                                SDIO_ICR_TXUNDERRC | \
                                SDIO_ICR_RXOVERRC  | \
                                SDIO_ICR_CMDRENDC  | \
                                SDIO_ICR_CMDSENTC  | \
                                SDIO_ICR_DATAENDC  | \
                                SDIO_ICR_STBITERRC | \
                                SDIO_ICR_DBCKENDC  | \
                                SDIO_ICR_SDIOITC   | \
                                SDIO_ICR_CEATAENDC


/* Driver Version */
static const ARM_DRV_VERSION DriverVersion = {
  ARM_MCI_API_VERSION,
  ARM_MCI_DRV_VERSION
};

/* Driver Capabilities */
static const ARM_MCI_CAPABILITIES DriverCapabilities = {
  (RTE_SDIO_CD_PIN != 0) ?
  1 : 0,    /* cd_state      */
  0,        /* cd_event      */
  (RTE_SDIO_WP_PIN != 0) ?
  1 : 0,    /* wp_state      */
  0,        /* busy_event    */
#if defined(STM32F10X_HD) || defined(STM32F10X_XL)
  1,        /* data_width_4  */
  1,        /* data_width_8  */
#else
  0,        /* data_width_4  */
  0,        /* data_width_8  */
#endif
  0,        /* power_off     */
  0,        /* vdd_1v8       */
  0,        /* signaling_1v8 */
  0         /* reserved      */
};

#if defined(STM32F10X_HD) || defined(STM32F10X_XL)
/* MCI Message Queue */
osMessageQDef(MCI_MsgQ, 4, uint32_t);
#endif

/* MCI Information */
static struct {
  bool                   Power;             // Power flag
  bool                   SPI_Mode;          // SPI mode
  ARM_MCI_SignalEvent_t  cb_event;          // Event Callback
#if defined(STM32F10X_HD) || defined(STM32F10X_XL)
  ARM_MCI_BUS_DATA_WIDTH DataWidth;         // Current bus data width
  uint32_t               DataReadTimeout;   // 100ms Timeout @ SD Bus Clock
  uint32_t               DataWriteTimeout;  // 250ms Timeout @ SD Bus Clock
  osMessageQId           MessageQId;        // Message Queue ID
#endif
} MCI_Info;


/**
  \fn          ARM_DRV_VERSION GetVersion (void)
  \brief       Get driver version.
  \return      \ref ARM_DRV_VERSION
*/
static ARM_DRV_VERSION GetVersion (void) {
  return DriverVersion;
}


/**
  \fn          ARM_MCI_CAPABILITIES MCI_GetCapabilities (void)
  \brief       Get driver capabilities.
  \return      \ref ARM_MCI_CAPABILITIES
*/
static ARM_MCI_CAPABILITIES GetCapabilities (void) {
  return DriverCapabilities;
}


/**
  \fn          ARM_MCI_STATUS MCI_Initialize (ARM_MCI_SignalEvent_t cb_event, bool spi_mode)
  \brief       Initialize the Memory Card Interface
  \param[in]   cb_event  Pointer to \ref ARM_MCI_SignalEvent
  \param[in]   spi_mode  SPI Mode (default is native MMC/SD Mode)
  \return      \ref ARM_MCI_STATUS execution status
*/
static ARM_MCI_STATUS Initialize (ARM_MCI_SignalEvent_t cb_event, bool spi_mode) {

  MCI_Info.cb_event = cb_event;

  /* Configure CD (Card Detect) Pin */
  #if (RTE_SDIO_CD_PIN)
    GPIO_PortClock    (RTE_SDIO_CD_PORT, true);
    GPIO_PinConfigure (RTE_SDIO_CD_PORT, RTE_SDIO_CD_BIT,
                      (RTE_SDIO_CD_ACTIVE == 0) ? GPIO_IN_PULL_UP : GPIO_IN_PULL_DOWN,
                       GPIO_MODE_INPUT);
  #endif

  /* Configure WP (Write Protect) Pin */
  #if (RTE_SDIO_WP_PIN)
    GPIO_PortClock    (RTE_SDIO_WP_PORT, true);
    GPIO_PinConfigure (RTE_SDIO_WP_PORT, RTE_SDIO_WP_BIT,
                      (RTE_SDIO_CD_ACTIVE == 0) ? GPIO_IN_PULL_UP : GPIO_IN_PULL_DOWN,
                       GPIO_MODE_INPUT);
  #endif

  MCI_Info.SPI_Mode = spi_mode;
  if (spi_mode == true) {
    return ARM_MCI_OK;
  }
  
#if defined(STM32F10X_HD) || defined(STM32F10X_XL)

  /* Enable SDIO peripheral clock */
  RCC->AHBENR |= RCC_AHBENR_SDIOEN;

  /* Initialize DMA channel */
  DMA_ChannelInitialize(RTE_SDIO_DMA_NUMBER, RTE_SDIO_DMA_CHANNEL);

  MCI_Info.Power = true;

  /* Create and initialize message queue */
  if (MCI_Info.MessageQId == NULL) {
    MCI_Info.MessageQId = osMessageCreate(osMessageQ (MCI_MsgQ), NULL);

    if (MCI_Info.MessageQId == NULL) {
      /* Message queue id not obtained */
      return ARM_MCI_ERROR;
    }
  }

  /* Enable SDIO interrupts */
  SDIO->MASK = SDIO_MASK_CMDRENDIE  |   /* Response received interrupt  */
               SDIO_MASK_CTIMEOUTIE |   /* Command timeout interrupt    */
               SDIO_MASK_CCRCFAILIE ;   /* Command CRC error int        */

  NVIC_ClearPendingIRQ(SDIO_IRQn);
  NVIC_EnableIRQ(SDIO_IRQn);

  /* Success, SDIO peripheral initialized */
  return ARM_MCI_OK;
#else
  /* Native mode unsupported */
  return ARM_MCI_ERROR_UNSUPPORTED;
#endif
}


/**
  \fn          ARM_MCI_STATUS MCI_Uninitialize (void)
  \brief       De-initialize Memory Card Interface.
  \return      \ref ARM_MCI_STATUS execution status
*/
static ARM_MCI_STATUS Uninitialize (void) {
  MCI_Info.Power = false;

  /* Unconfigure CD (Card Detect) Pin */
  #if (RTE_SDIO_CD_PIN)
    GPIO_PinConfigure(RTE_SDIO_CD_PORT, RTE_SDIO_CD_BIT, GPIO_IN_ANALOG,
                                                         GPIO_MODE_INPUT);
  #endif

  /* Unconfigure WP (Write Protect) Pin */
  #if (RTE_SDIO_WP_PIN)
    GPIO_PinConfigure(RTE_SDIO_WP_PORT, RTE_SDIO_WP_BIT, GPIO_IN_ANALOG,
                                                         GPIO_MODE_INPUT);
  #endif

  if (MCI_Info.SPI_Mode == true) {
    return ARM_MCI_OK;
  }

#if defined(STM32F10X_HD) || defined(STM32F10X_XL)

  /* Disable SDIO peripheral clock */
  RCC->AHBENR &= ~RCC_AHBENR_SDIOEN;

  /* DMA channel uninit */
  DMA_ChannelUninitialize(RTE_SDIO_DMA_NUMBER, RTE_SDIO_DMA_CHANNEL);

  /* Disable SDIO interrupts */
  NVIC_DisableIRQ(SDIO_IRQn);

  /* Release waiting threads */
  osMessagePut(MCI_Info.MessageQId, MCI_MSG_ABORT, 0);

  /* Clear message queue */
  while (osMessageGet(MCI_Info.MessageQId, 0).status == osEventMessage){;}

  /* Unconfigure CK and CMD */
  GPIO_PinConfigure(RTE_SDIO_CK_PORT, RTE_SDIO_CK_PIN, GPIO_IN_ANALOG,
                                                       GPIO_MODE_INPUT);
  GPIO_PinConfigure(RTE_SDIO_CMD_PORT, RTE_SDIO_CMD_PIN, GPIO_IN_ANALOG,
                                                         GPIO_MODE_INPUT);

  switch (MCI_Info.DataWidth) {
    case ARM_MCI_BUS_DATA_WIDTH_8:

      /* D7..4 */
      GPIO_PinConfigure(RTE_SDIO_D7_PORT, RTE_SDIO_D7_PIN, GPIO_IN_ANALOG,
                                                           GPIO_MODE_INPUT);
      GPIO_PinConfigure(RTE_SDIO_D6_PORT, RTE_SDIO_D6_PIN, GPIO_IN_ANALOG,
                                                           GPIO_MODE_INPUT);
      GPIO_PinConfigure(RTE_SDIO_D5_PORT, RTE_SDIO_D5_PIN, GPIO_IN_ANALOG,
                                                           GPIO_MODE_INPUT);
      GPIO_PinConfigure(RTE_SDIO_D4_PORT, RTE_SDIO_D4_PIN, GPIO_IN_ANALOG,
                                                           GPIO_MODE_INPUT);

    case ARM_MCI_BUS_DATA_WIDTH_4:

      /* D3..1 */
      GPIO_PinConfigure(RTE_SDIO_D3_PORT, RTE_SDIO_D3_PIN, GPIO_IN_ANALOG,
                                                           GPIO_MODE_INPUT);
      GPIO_PinConfigure(RTE_SDIO_D2_PORT, RTE_SDIO_D2_PIN, GPIO_IN_ANALOG,
                                                           GPIO_MODE_INPUT);
      GPIO_PinConfigure(RTE_SDIO_D1_PORT, RTE_SDIO_D1_PIN, GPIO_IN_ANALOG,
                                                           GPIO_MODE_INPUT);

    case ARM_MCI_BUS_DATA_WIDTH_1:

      /* D0 */
      GPIO_PinConfigure(RTE_SDIO_D0_PORT, RTE_SDIO_D0_PIN, GPIO_IN_ANALOG,
                                                           GPIO_MODE_INPUT);
      break;

    default:
      return ARM_MCI_ERROR_UNSUPPORTED;
  }

  return ARM_MCI_OK;
#else
  /* Native mode unsupported */
  return ARM_MCI_ERROR_UNSUPPORTED;
#endif
}


/**
  \fn          ARM_MCI_STATUS MCI_PowerControl (ARM_POWER_STATE state)
  \brief       Control Memory Card Interface Power.
  \param[in]   state   Power state \ref ARM_POWER_STATE
  \return      \ref ARM_MCI_STATUS execution status
*/
static ARM_MCI_STATUS PowerControl (ARM_POWER_STATE state) {

  switch (state) {
    case ARM_POWER_OFF:
    #if defined(STM32F10X_HD) || defined(STM32F10X_XL)

      /* Disable DMA and clear data transfer bit */
      SDIO->DCTRL &= ~(SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN);
  
      /* Clear SDIO FIFO */
      while (SDIO->FIFOCNT) {
        SDIO->FIFO;
      }

      /* Clear SDIO interrupts */
      SDIO->ICR = SDIO_ICR_BIT_MASK;

      /* Power-off card */
      SDIO->POWER = 0;

      /* Disable SDIO peripheral clock */
      RCC->AHBENR &= ~RCC_AHBENR_SDIOEN;

      DMA_ChannelDisable(DMAx_CHANNELy(RTE_SDIO_DMA_NUMBER, RTE_SDIO_DMA_CHANNEL));

      MCI_Info.Power = false;

      /* Release waiting threads */
      osMessagePut(MCI_Info.MessageQId, MCI_MSG_ABORT, 0);

      /* Clear message queue */
      while (osMessageGet(MCI_Info.MessageQId, 0).status == osEventMessage);
    #else
      MCI_Info.Power = false;
    #endif

      break;

    case ARM_POWER_LOW:
      return ARM_MCI_ERROR_UNSUPPORTED;
      
    case ARM_POWER_FULL:
    #if defined(STM32F10X_HD) || defined(STM32F10X_XL)

      /* Enable SDIO peripheral clock */
      RCC->AHBENR |= RCC_AHBENR_SDIOEN;

      /* Power-on, the card is clocked */
      SDIO->POWER = SDIO_POWER_PWRCTRL_1 | SDIO_POWER_PWRCTRL_0;
    #endif
      MCI_Info.Power = true;

      break;

    default:
      return ARM_MCI_ERROR_UNSUPPORTED;
  }
  return ARM_MCI_OK;
}


#if defined(STM32F10X_HD) || defined(STM32F10X_XL)
/**
  \fn          ARM_MCI_STATUS MCI_BusMode (ARM_MCI_BUS_MODE mode)
  \brief       Setup the bus mode for Memory Card transfers.
  \param[in]   mode  Requested bus mode
  \return      \ref ARM_MCI_STATUS execution status
*/
static ARM_MCI_STATUS BusMode (ARM_MCI_BUS_MODE mode) {

  switch (mode) {
    case ARM_MCI_BUS_OPEN_DRAIN:
      /* Configure command line in open-drain mode */
      GPIO_PinConfigure(RTE_SDIO_CMD_PORT, RTE_SDIO_CMD_PIN, GPIO_AF_OPENDRAIN,
                                                             GPIO_MODE_OUT50MHZ);
      break;
    case ARM_MCI_BUS_PUSH_PULL:
      /* Configure command line in push-pull mode */
      GPIO_PinConfigure(RTE_SDIO_CMD_PORT, RTE_SDIO_CMD_PIN, GPIO_AF_PUSHPULL,
                                                             GPIO_MODE_OUT50MHZ);
      break;
    default:
      return ARM_MCI_ERROR_UNSUPPORTED;
  }

  return ARM_MCI_OK;
}
#endif


#if defined(STM32F10X_HD) || defined(STM32F10X_XL)
/**
  \fn          ARM_MCI_STATUS MCI_BusDataWidth (ARM_MCI_BUS_DATA_WIDTH width)
  \brief       Setup bus data width for Memory Card transfers.
  \param[in]   width  Requested bus data width
  \return      \ref ARM_MCI_STATUS execution status
*/
static ARM_MCI_STATUS BusDataWidth (ARM_MCI_BUS_DATA_WIDTH width) {

  if (!MCI_Info.Power) return ARM_MCI_ERROR;

  SDIO->CLKCR &= ~SDIO_CLKCR_WIDBUS;

  switch (width) {
    case ARM_MCI_BUS_DATA_WIDTH_1:
      break;
    case ARM_MCI_BUS_DATA_WIDTH_4:
      SDIO->CLKCR |= SDIO_CLKCR_WIDBUS_0;
      break;    
    case ARM_MCI_BUS_DATA_WIDTH_8:
      SDIO->CLKCR |= SDIO_CLKCR_WIDBUS_1;
      break;
    default:
      return ARM_MCI_ERROR_UNSUPPORTED;
  }

  MCI_Info.DataWidth = width;

  /* Enable GPIOB, GPIOC and GPIOD port clock */
  GPIO_PortClock(GPIOB, true);
  GPIO_PortClock(GPIOC, true);
  GPIO_PortClock(GPIOD, true);

  /* Configure CK and CMD */
  GPIO_PinConfigure(RTE_SDIO_CK_PORT, RTE_SDIO_CK_PIN, GPIO_AF_PUSHPULL,
                                                       GPIO_MODE_OUT50MHZ);
  GPIO_PinConfigure(RTE_SDIO_CMD_PORT, RTE_SDIO_CMD_PIN, GPIO_AF_PUSHPULL,
                                                         GPIO_MODE_OUT50MHZ);

  switch (width) {
    case ARM_MCI_BUS_DATA_WIDTH_8:

      /* D7..4 */
      GPIO_PinConfigure(RTE_SDIO_D7_PORT, RTE_SDIO_D7_PIN, GPIO_AF_PUSHPULL,
                                                           GPIO_MODE_OUT50MHZ);
      GPIO_PinConfigure(RTE_SDIO_D6_PORT, RTE_SDIO_D6_PIN, GPIO_AF_PUSHPULL,
                                                           GPIO_MODE_OUT50MHZ);
      GPIO_PinConfigure(RTE_SDIO_D5_PORT, RTE_SDIO_D5_PIN, GPIO_AF_PUSHPULL,
                                                           GPIO_MODE_OUT50MHZ);
      GPIO_PinConfigure(RTE_SDIO_D4_PORT, RTE_SDIO_D4_PIN, GPIO_AF_PUSHPULL,
                                                           GPIO_MODE_OUT50MHZ);

    case ARM_MCI_BUS_DATA_WIDTH_4:

      /* D3..1 */
      GPIO_PinConfigure(RTE_SDIO_D3_PORT, RTE_SDIO_D3_PIN, GPIO_AF_PUSHPULL,
                                                           GPIO_MODE_OUT50MHZ);
      GPIO_PinConfigure(RTE_SDIO_D2_PORT, RTE_SDIO_D2_PIN, GPIO_AF_PUSHPULL,
                                                           GPIO_MODE_OUT50MHZ);
      GPIO_PinConfigure(RTE_SDIO_D1_PORT, RTE_SDIO_D1_PIN, GPIO_AF_PUSHPULL,
                                                           GPIO_MODE_OUT50MHZ);

    case ARM_MCI_BUS_DATA_WIDTH_1:

      /* D0 */
      GPIO_PinConfigure(RTE_SDIO_D0_PORT, RTE_SDIO_D0_PIN, GPIO_AF_PUSHPULL,
                                                           GPIO_MODE_OUT50MHZ);
      break;

    default:
      return ARM_MCI_ERROR_UNSUPPORTED;
  }

  return ARM_MCI_OK;
}
#endif


#if defined(STM32F10X_HD) || defined(STM32F10X_XL)
/**
  \fn          ARM_MCI_STATUS MCI_BusSignaling (ARM_MCI_BUS_SIGNALING voltage)
  \brief       Set bus signaling voltage for MCI transfers.
  \param[in]   voltage  Requested signaling voltage
  \return      \ref ARM_MCI_STATUS execution status
*/
static ARM_MCI_STATUS BusSignaling (ARM_MCI_BUS_SIGNALING voltage) {
  return ARM_MCI_ERROR_UNSUPPORTED;
}
#endif


#if defined(STM32F10X_HD) || defined(STM32F10X_XL)
/**
  \fn          uint32_t MCI_BusSpeed (uint32_t bps)
  \brief       Setup bus speed for Memory Card transfers.
  \param[in]   bps  Requested speed in bits/s
  \return      Configured bus speed in bits/s
*/
static uint32_t BusSpeed (uint32_t bps) {
  uint32_t div;

  if (!MCI_Info.Power) return 0;

  /* bps = SDIOCLK / (div + 2) */
  div = (RTE_HCLK + bps - 1) / bps;
  if (div < 2)                 div  = 0;
  else                         div -= 2;
  if (div > SDIO_CLKCR_CLKDIV) div  = SDIO_CLKCR_CLKDIV;
  SDIO->CLKCR = (SDIO->CLKCR & ~SDIO_CLKCR_CLKDIV)   |
                SDIO_CLKCR_PWRSAV | SDIO_CLKCR_CLKEN | div;

  bps = RTE_HCLK / (div + 2);

  /* Set timeouts */
  MCI_Info.DataReadTimeout  = 100 * (bps / 1000);       /* 100ms */
  MCI_Info.DataWriteTimeout = 250 * (bps / 1000);       /* 250ms */

  return bps;
}
#endif


#if defined(STM32F10X_HD) || defined(STM32F10X_XL)
/**
  \fn          ARM_MCI_STATUS MCI_CardPower (ARM_MCI_POWER voltage)
  \brief       Set Memory Card supply voltage.
  \param[in]   voltage  Memory Card supply voltage \ref ARM_MCI_POWER
  \return      \ref ARM_MCI_STATUS execution status
*/
static ARM_MCI_STATUS CardPower (ARM_MCI_POWER voltage) {
  return ARM_MCI_ERROR_UNSUPPORTED;
}
#endif


/**
  \fn          ARM_MCI_SWITCH CardSwitchRead (void)
  \brief       Read state of Memory Card switches.
  \return      \ref ARM_MCI_SWITCH execution status
*/
static ARM_MCI_SWITCH CardSwitchRead (void) {
  ARM_MCI_SWITCH sw = { 1, 0 };

  /* Read CD (Card Detect) Pin */
  #if (RTE_SDIO_CD_PIN)
    sw.cd_state = GPIO_PinRead(RTE_SDIO_CD_PORT, RTE_SDIO_CD_BIT) == RTE_SDIO_CD_ACTIVE;
  #endif

  /* Read WP (Write Protect) Pin */
  #if (RTE_SDIO_WP_PIN)
    sw.wp_state = GPIO_PinRead(RTE_SDIO_WP_PORT, RTE_SDIO_WP_BIT) == RTE_SDIO_WP_ACTIVE;
  #endif

  return (sw);
}


#if defined(STM32F10X_HD) || defined(STM32F10X_XL)
/**
  \fn          ARM_MCI_STATUS MCI_CardInitialize (void)
  \brief       Perform Memory Card initialization sequence.
  \return      \ref ARM_MCI_STATUS execution status
*/
static ARM_MCI_STATUS CardInitialize (void) {

  if (!MCI_Info.Power) return ARM_MCI_ERROR;

  /* Power-on, the card is clocked */
  SDIO->POWER = SDIO_POWER_PWRCTRL_1 | SDIO_POWER_PWRCTRL_0;
  
  /* Send at least 74 SD clocks to the SD card */
  SDIO->CLKCR = (SDIO->CLKCR & ~(SDIO_CLKCR_CLKDIV | SDIO_CLKCR_PWRSAV)) | SDIO_CLKCR_CLKEN;
  osDelay(2);

  /* Stop the clock */
  SDIO->CLKCR &= ~SDIO_CLKCR_CLKEN;

  return ARM_MCI_OK;
}
#endif


#if defined(STM32F10X_HD) || defined(STM32F10X_XL)
/**
  \fn          ARM_MCI_STATUS MCI_SendCommand (ARM_MC_COMMAND   cmd,
                                               uint32_t         cmd_arg,
                                               ARM_MC_RESPONSE  response_type,
                                               uint32_t        *ptr_response)
  \brief       Send Command to card and get the response.
  \param[in]   cmd            Memory Card command \ref ARM_MC_COMMAND
  \param[in]   cmd_arg        Command argument
  \param[in]   response_type  Expected response type
  \param[out]  ptr_response   Pointer to response
  \return      \ref ARM_MCI_STATUS execution status
*/
static ARM_MCI_STATUS SendCommand (ARM_MC_COMMAND cmd, uint32_t cmd_arg, ARM_MC_RESPONSE response_type, uint32_t *ptr_response) {
  uint32_t cmd_val, status, tick;
  osEvent  evt;

  if (!MCI_Info.Power) return ARM_MCI_ERROR;

  if ((response_type != ARM_MC_RESPONSE_NONE) && (ptr_response == NULL)) {
    return ARM_MCI_ERROR;
  }

  switch (response_type) {
    case ARM_MC_RESPONSE_NONE:
      /* No response, expect CMDSENT flag */
      cmd_val = 0;
      break;

    case ARM_MC_RESPONSE_R2:
      /* Long response, expect CMDREND or CCRCFAIL flag */
      cmd_val = SDIO_CMD_WAITRESP_1 | SDIO_CMD_WAITRESP_0;
      break;

    default:
      /* Short response, expect CMDREND or CCRCFAIL flag */
      cmd_val = SDIO_CMD_WAITRESP_0;
      break;
  }

  cmd_val |= cmd | SDIO_CMD_CPSMEN;

  /* Clear all interrupt flags */
  SDIO->ICR = SDIO_ICR_BIT_MASK;

  /* Send the command */
  SDIO->ARG = cmd_arg;
  SDIO->CMD = cmd_val;
  
  if (response_type == ARM_MC_RESPONSE_NONE) {
    /* Wait until command finished */
    tick = osKernelSysTick();
    do {
      if (!MCI_Info.Power) break;
      if (SDIO->STA & SDIO_STA_CMDSENT) {
        return ARM_MCI_OK;
      }
    } while ((osKernelSysTick() - tick) < osKernelSysTickMicroSec(MCI_CMD_SEND_TIMEOUT));
    return ARM_MCI_ERROR_COMMAND_TIMEOUT;
  }

  /* Wait for message with timeout */
  evt = osMessageGet(MCI_Info.MessageQId, MCI_CMD_RESPONSE_TIMEOUT);

  if (evt.status == osEventTimeout) {
    /* Timeout, no message received */
    return ARM_MCI_ERROR_COMMAND_TIMEOUT;
  }

  /* Check received message, handle errors */
  status = evt.value.v;

  if (status & MCI_MSG_ABORT) {
    return ARM_MCI_ERROR_COMMAND_TIMEOUT;
  }

  if (status & SDIO_STA_CTIMEOUT) {
    /* Command response timeout */
    return ARM_MCI_ERROR_COMMAND_TIMEOUT;
  }

  if (status & SDIO_STA_CCRCFAIL) {
    if ((cmd == ARM_MC_CMD_SEND_OP_COND)     ||
        (cmd == ARM_MC_CMD_SD_SEND_OP_COND)  ||
        (cmd == ARM_MC_CMD_STOP_TRANSMISSION)) {
      /* Invalid detection of the CRC error */
      SDIO->CMD = 0;
    }
    else {
      /* Command response CRC error */
      return ARM_MCI_ERROR_CRC;
    }
  }

  if (status & SDIO_STA_CMDREND) {
    /* Command response received, CRC check passed */
    if ((SDIO->RESPCMD & SDIO_RESPCMD_RESPCMD) != cmd) {
      if ((SDIO->RESPCMD & SDIO_RESPCMD_RESPCMD) != SDIO_RESPCMD_RESPCMD) {
        /* This is not a response to the last command! */
        return ARM_MCI_ERROR_RESPONSE;
      }
    }
  }

  /* Read MCI response registers */
  ptr_response[0] = SDIO->RESP1;
  if (response_type == ARM_MC_RESPONSE_R2) {
    ptr_response[1] = SDIO->RESP2;
    ptr_response[2] = SDIO->RESP3;
    ptr_response[3] = SDIO->RESP4;
  }

  /* Command sent */
  return ARM_MCI_OK;
}
#endif


#if defined(STM32F10X_HD) || defined(STM32F10X_XL)
/**
  \fn          ARM_MCI_STATUS MCI_SetupTransfer (uint8_t *data,
                                                 uint32_t block_count,
                                                 uint32_t block_size,
                                                 ARM_MCI_TRANSFER_MODE mode)
  \brief         Setup read or write transfer operation.
  \param[in,out] data         Pointer to data block(s) to be written or read
  \param[in]     block_count  Number of blocks
  \param[in]     block_size   Size of a block in bytes
  \param[in]     mode         Read or write transfer mode
  \return        \ref ARM_MCI_STATUS execution status
*/
static ARM_MCI_STATUS SetupTransfer (uint8_t *data, uint32_t block_count, uint32_t block_size, ARM_MCI_TRANSFER_MODE mode) {
  uint32_t ch_cfg;

  if ((data == NULL) || (block_count == 0) || (block_size == 0)) return ARM_MCI_ERROR;

  if (!MCI_Info.Power) return ARM_MCI_ERROR;

  /* Configure DMA channel */
  ch_cfg = DMA_PERIPHERAL_TO_MEMORY                     |
           (RTE_SDIO_DMA_PRIORITY << DMA_PRIORITY_POS)  |
           DMA_MEMORY_DATA_32BIT                        |
           DMA_PERIPHERAL_DATA_32BIT                    |
           DMA_MEMORY_INCREMENT                         |
           DMA_TRANSFER_COMPLETE_INTERRUPT              ;

  switch (mode) {
    case ARM_MCI_TRANSFER_READ:
      break;
    case ARM_MCI_TRANSFER_WRITE:
      ch_cfg |= DMA_READ_MEMORY;
      break;
    default:
      return ARM_MCI_ERROR; 
  }

  DMA_ChannelConfigure(DMAx_CHANNELy(RTE_SDIO_DMA_NUMBER, RTE_SDIO_DMA_CHANNEL),
                      ch_cfg,
                      (uint32_t)&(SDIO->FIFO),          /* Peripheral address */
                      (uint32_t) data,                  /* Memory address */
                      block_count * block_size / 4);    /* Number of data bytes */

  DMA_ChannelEnable(DMAx_CHANNELy(RTE_SDIO_DMA_NUMBER, RTE_SDIO_DMA_CHANNEL));

  return (ARM_MCI_OK);
}
#endif


#if defined(STM32F10X_HD) || defined(STM32F10X_XL)
/**
  \fn          ARM_MCI_STATUS MCI_ReadTransfer  (uint8_t *data,
                                                 uint32_t block_count,
                                                 uint32_t block_size)
  \brief       Perfrom read transfer.
  \param[out]  data         Pointer to data block(s) to be read
  \param[in]   block_count  Number of blocks
  \param[in]   block_size   Size of a block in bytes
  \return      \ref ARM_MCI_STATUS execution status
*/
static ARM_MCI_STATUS ReadTransfer (uint8_t *data, uint32_t block_count, uint32_t block_size) {
  uint32_t n, db_sz;
  osEvent  evt;

  if ((data == NULL) || (block_count == 0) || (block_size == 0)) return ARM_MCI_ERROR;

  if (!MCI_Info.Power) return ARM_MCI_ERROR;
  
  if (block_size == 512) {
    db_sz = 9;
  }
  else {
    if (block_size > 16384) {
      return ARM_MCI_ERROR_UNSUPPORTED;
    }
    for (n = 0; n < 14; n++) {
      if (block_size & (1 << n)) {
        break;
      }
    }
    db_sz = n;
  }

  SDIO->DTIMER = MCI_Info.DataReadTimeout;
  SDIO->DLEN   = block_count * block_size;

  /* Trigger (DMA) data transfer */
  SDIO->DCTRL = (db_sz << 4)     |
                SDIO_DCTRL_DMAEN |
                SDIO_DCTRL_DTDIR |
                SDIO_DCTRL_DTEN;

  evt = osMessageGet(MCI_Info.MessageQId, block_count * MCI_READ_TRANSFER_TIMEOUT);

  if (evt.status == osEventMessage) {
    if (evt.value.v & MCI_MSG_ABORT) {
      return ARM_MCI_ERROR_TRANSFER_TIMEOUT;
    }
    if (evt.value.v & MCI_MSG_DMA_DONE) {
      /* Data transfer finished. */
      if (SDIO->STA & SDIO_STA_DCRCFAIL) {
        /* Data block received, CRC check failed */
        return ARM_MCI_ERROR_CRC;
      }
      return ARM_MCI_OK;
    }
  }

  /* Transfer did not complete in given time or some other error */
  return ARM_MCI_ERROR_TRANSFER_TIMEOUT;
}
#endif


#if defined(STM32F10X_HD) || defined(STM32F10X_XL)
/**
  \fn          ARM_MCI_STATUS MCI_WriteTransfer (uint8_t *data,
                                                 uint32_t block_count,
                                                 uint32_t block_size)
  \brief       Perfrom write transfer.
  \param[in]   data         Pointer to data block(s) to be written
  \param[in]   block_count  Number of blocks
  \param[in]   block_size   Size of a block in bytes
  \return      \ref ARM_MCI_STATUS execution status
*/
static ARM_MCI_STATUS WriteTransfer (uint8_t *data, uint32_t block_count, uint32_t block_size) {
  uint32_t n, db_sz;
  osEvent  evt;
  uint32_t tick;

  if ((data == NULL) || (block_count == 0) || (block_size == 0)) return ARM_MCI_ERROR;

  if (!MCI_Info.Power) return ARM_MCI_ERROR;

  if (block_size == 512) {
    db_sz = 9;
  }
  else {
    if (block_size > 16384) {
      return ARM_MCI_ERROR_UNSUPPORTED;
    }
    for (n = 0; n < 14; n++) {
      if (block_size & (1 << n)) {
        break;
      }
    }
    db_sz = n;
  }

  SDIO->DTIMER = MCI_Info.DataWriteTimeout;
  SDIO->DLEN   = block_count * block_size;

  /* Trigger (DMA) data transfer */
  SDIO->DCTRL  = (db_sz << 4)     |
                 SDIO_DCTRL_DMAEN |
                 SDIO_DCTRL_DTEN;

  evt = osMessageGet(MCI_Info.MessageQId, block_count * MCI_WRITE_TRANSFER_TIMEOUT);

  if (evt.status == osEventMessage) {
    if (evt.value.v & MCI_MSG_ABORT) {
      return ARM_MCI_ERROR_TRANSFER_TIMEOUT;
    }
    if (evt.value.v & MCI_MSG_DMA_DONE) {
      /* DMA transfer finished, wait until data block sent to card */
      tick = osKernelSysTick();
      do {
        if ((SDIO->STA & (SDIO_STA_DATAEND | SDIO_STA_DBCKEND)) == (SDIO_STA_DATAEND | SDIO_STA_DBCKEND)) {
          if (SDIO->STA & (SDIO_STA_DCRCFAIL | SDIO_STA_DTIMEOUT)) {
            /* Error during data transfer */
            return ARM_MCI_ERROR_CRC;
          }
          return ARM_MCI_OK;
        }
      } while ((osKernelSysTick() - tick) < osKernelSysTickMicroSec(MCI_CMD_SEND_TIMEOUT));
    }
  }

  /* Transfer did not complete in given time or some other error */
  return ARM_MCI_ERROR_TRANSFER_TIMEOUT;
}
#endif


#if defined(STM32F10X_HD) || defined(STM32F10X_XL)
/**
  \fn          ARM_MCI_STATUS MCI_AbortTransfer (void)
  \brief       Abort current read/write data transfer.
  \return      \ref ARM_MCI_STATUS execution status
*/
static ARM_MCI_STATUS AbortTransfer (void) {

  if (!MCI_Info.Power) return ARM_MCI_ERROR;

  /* Disable DMA and clear data transfer bit */
  SDIO->DCTRL &= ~(SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN);
  
  /* Clear SDIO FIFO */
  while (SDIO->FIFOCNT) {
    SDIO->FIFO;
  }

  /* Clear SDIO interrupts */
  SDIO->ICR = SDIO_ICR_BIT_MASK;

  DMA_ChannelDisable(DMAx_CHANNELy(RTE_SDIO_DMA_NUMBER, RTE_SDIO_DMA_CHANNEL));

  /* Release waiting threads */
  osMessagePut(MCI_Info.MessageQId, MCI_MSG_ABORT, 0);

  /* Clear message queue */
  while (osMessageGet(MCI_Info.MessageQId, 0).status == osEventMessage);

  return ARM_MCI_OK;
}
#endif


#if defined(STM32F10X_HD) || defined(STM32F10X_XL)
/* DMA Channel Event */
void DMA2_Channel4_Event (uint32_t event) {

  DMA_ChannelDisable(DMAx_CHANNELy(RTE_SDIO_DMA_NUMBER, RTE_SDIO_DMA_CHANNEL));

  if (event & DMA_CHANNEL_TRANSFER_COMPLETE) {
    /* Data transfer finished */
    osMessagePut(MCI_Info.MessageQId, MCI_MSG_DMA_DONE, 0);
  } else {
    /* Stream error */
    osMessagePut(MCI_Info.MessageQId, MCI_MSG_DMA_ERROR, 0);
  }
}
#endif


#if defined(STM32F10X_HD) || defined(STM32F10X_XL)
/* SDIO IRQ Handler */
void SDIO_IRQHandler (void) {
  uint32_t status;

  /* Read SDIO interrupt status */
  status = SDIO->STA;

  if (status & (SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL | SDIO_STA_CMDREND)) {
    SDIO->ICR = status & (SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL | SDIO_STA_CMDREND);
    /* Send status as message */
    osMessagePut(MCI_Info.MessageQId, status, 0);
  }
}
#endif


/* MCI Driver Control Block */
ARM_DRIVER_MCI Driver_MCI0 = {
  GetVersion,
  GetCapabilities,
  Initialize,
  Uninitialize,
  PowerControl,
#if defined(STM32F10X_HD) || defined(STM32F10X_XL)
  BusMode,
  BusDataWidth,
  BusSignaling,
  BusSpeed,
  CardPower,
  CardSwitchRead,
  CardInitialize,
  SendCommand,
  SetupTransfer,
  ReadTransfer,
  WriteTransfer,
  AbortTransfer
#else
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  CardSwitchRead,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
#endif
};
