//*****************************************************************************
//
//! @file radio_task.c
//!
//! @brief Task to handle radio operation.
//!
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2020, Ambiq Micro
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// Third party software included in this distribution is subject to the
// additional license terms as defined in the /docs/licenses directory.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// This is part of revision v2.4.2-1241-g15e33e803 of the AmbiqSuite Development Package.
//
//*****************************************************************************

//*****************************************************************************
//
// Global includes for this project.
//
//*****************************************************************************
#include "ble_freertos_amdtpc.h"

//*****************************************************************************
//
// WSF standard includes.
//
//*****************************************************************************
#include "wsf_types.h"
#include "wsf_trace.h"
#include "wsf_buf.h"
#include "wsf_timer.h"

//*****************************************************************************
//
// Includes for operating the ExactLE stack.
//
//*****************************************************************************
#include "hci_handler.h"
#include "dm_handler.h"
#include "l2c_handler.h"
#include "att_handler.h"
#include "smp_handler.h"
#include "l2c_api.h"
#include "att_api.h"
#include "smp_api.h"
#include "app_api.h"
#include "hci_core.h"
#include "hci_drv.h"
#include "hci_drv_apollo.h"
#include "hci_drv_cooper.h"

#include "am_mcu_apollo.h"
#include "am_util.h"
#include "am_bsp.h"

#include "hci_apollo_config.h"
#include "wsf_msg.h"

//*****************************************************************************
//
// Includes for the AMDTP profile.
//
//*****************************************************************************
#include "amdtp_api.h"
#include "amdtpc_api.h"
#include "app_ui.h"

#ifdef BLE_MENU
#include "ble_menu.h"
#endif

//*****************************************************************************
//
// Radio task handle.
//
//*****************************************************************************
TaskHandle_t radio_task_handle;

//*****************************************************************************
//
// Function prototypes
//
//*****************************************************************************
void exactle_stack_init(void);

//*****************************************************************************
//
// WSF buffer pools.
//
//*****************************************************************************
#define WSF_BUF_POOLS               4
volatile uint32_t g_ui32UARTRxIndex = 0;

// Important note: the size of g_pui32BufMem should includes both overhead of internal
// buffer management structure, wsfBufPool_t (up to 16 bytes for each pool), and pool
// description (e.g. g_psPoolDescriptors below).

// Memory for the buffer pool
// extra AMOTA_PACKET_SIZE bytes for OTA handling
#if defined(USE_NONBLOCKING_HCI)
AM_SHARED_RW static uint32_t g_pui32BufMem[
        (WSF_BUF_POOLS*16
         + 16*8 + 32*4 + 64*6 + 280*8) / sizeof(uint32_t)];
#else
static uint32_t g_pui32BufMem[
        (WSF_BUF_POOLS*16
         + 16*8 + 32*4 + 64*6 + 280*8) / sizeof(uint32_t)];
#endif
// Default pool descriptor.
static wsfBufPoolDesc_t g_psPoolDescriptors[WSF_BUF_POOLS] =
{
    {  16,  8 },
    {  32,  4 },
    {  64,  6 },
    { 280,  8 }
};

#ifdef BLE_MENU
wsfHandlerId_t g_uartDataReadyHandlerId;
void uart_data_ready_handler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
    BleMenuRx();
}
#endif

#define am_uart_buffer(A)                                                   \
union                                                                   \
  {                                                                       \
    uint32_t words[(A + 3) >> 2];                                       \
      uint8_t bytes[A];                                                   \
  }

am_uart_buffer(1024) g_psWriteData;
am_uart_buffer(1024) g_psReadData;
volatile bool g_bRxTimeoutFlag = false;
//*****************************************************************************
//
// Tracking variable for the scheduler timer.
//
//*****************************************************************************

void radio_timer_handler(void);



//*****************************************************************************
//
// Initialization for the ExactLE stack.
//
//*****************************************************************************
void
exactle_stack_init(void)
{
    wsfHandlerId_t handlerId;
    uint16_t       wsfBufMemLen;
    //
    // Set up timers for the WSF scheduler.
    //
    WsfOsInit();
    WsfTimerInit();

    //
    // Initialize a buffer pool for WSF dynamic memory needs.
    //
    wsfBufMemLen = WsfBufInit(sizeof(g_pui32BufMem), (uint8_t *)g_pui32BufMem, WSF_BUF_POOLS,
               g_psPoolDescriptors);

    if (wsfBufMemLen > sizeof(g_pui32BufMem))
    {
        am_util_debug_printf("Memory pool is too small by %d\r\n",
                             wsfBufMemLen - sizeof(g_pui32BufMem));
    }

    //
    // Initialize the WSF security service.
    //
    SecInit();
    SecAesInit();
    SecCmacInit();
    SecEccInit();

    //
    // Set up callback functions for the various layers of the ExactLE stack.
    //
    handlerId = WsfOsSetNextHandler(HciHandler);
    HciHandlerInit(handlerId);

    handlerId = WsfOsSetNextHandler(DmHandler);
    DmDevVsInit(0);
    DmAdvInit();
    DmScanInit();
    DmConnInit();
    DmConnMasterInit();
    DmSecInit();
    DmSecLescInit();
    DmPrivInit();
    DmHandlerInit(handlerId);

    L2cInit();
    L2cMasterInit();

    handlerId = WsfOsSetNextHandler(AttHandler);
    AttHandlerInit(handlerId);
    AttsInit();
    AttsIndInit();
    AttcInit();

    handlerId = WsfOsSetNextHandler(SmpHandler);
    SmpHandlerInit(handlerId);
    SmpiInit();
    SmpiScInit();
    HciSetMaxRxAclLen(251);

    handlerId = WsfOsSetNextHandler(AppHandler);
    AppHandlerInit(handlerId);

    handlerId = WsfOsSetNextHandler(AmdtpcHandler);
    AmdtpcHandlerInit(handlerId);

    handlerId = WsfOsSetNextHandler(HciDrvHandler);
    HciDrvHandlerInit(handlerId);

#ifdef BLE_MENU
    g_uartDataReadyHandlerId = WsfOsSetNextHandler(uart_data_ready_handler);
#endif
}

//*****************************************************************************
//
// GPIO interrupt handler.
//
//*****************************************************************************
void
am_cooper_irq_isr(void)
{
    uint32_t    ui32IntStatus;

    am_hal_gpio_interrupt_irq_status_get(AM_COOPER_IRQn, false, &ui32IntStatus);
    am_hal_gpio_interrupt_irq_clear(AM_COOPER_IRQn, ui32IntStatus);
    am_hal_gpio_interrupt_service(AM_COOPER_IRQn,ui32IntStatus);
}

#ifdef BLE_MENU
//*****************************************************************************
//
// UART interrupt handler.
//
//*****************************************************************************
void
am_uart_isr(void)
{
    uint32_t ui32Status;
    char rxData;
    uint32_t ui32BytesRead;

    //
    // Read the masked interrupt status from the UART.
    //
    am_hal_uart_interrupt_status_get(UART, &ui32Status, true);
    am_hal_uart_interrupt_clear(UART, ui32Status);
    if (ui32Status & (AM_HAL_UART_INT_RX_TMOUT | AM_HAL_UART_INT_RX))
    {
        //
        // Service the uart FIFO.
        //
        const am_hal_uart_transfer_t sGetChar =
        {
            .eType = AM_HAL_UART_BLOCKING_READ,
            .pui8Data = (uint8_t *) &rxData,
            .ui32NumBytes = 1,
            .ui32TimeoutMs = AM_HAL_UART_WAIT_FOREVER,
            .pui32BytesTransferred = &ui32BytesRead,
        };

        am_hal_uart_transfer(UART, &sGetChar);

        if ((rxData == '\n') || (rxData == '\r'))
        {
            wsfMsgHdr_t  *pMsg;
            if ( (pMsg = WsfMsgAlloc(0)) != NULL )
            {
                WsfMsgSend(g_uartDataReadyHandlerId, pMsg);
            }
        }
        else
        {
            menuRxData[menuRxDataLen++] = rxData;
        }
    }
}
#endif

//*****************************************************************************
//
// Perform initial setup for the radio task.
//
//*****************************************************************************
void
RadioTaskSetup(void)
{
    am_util_debug_printf("RadioTask: setup\r\n");

    NVIC_SetPriority(COOPER_IOM_IRQn, NVIC_configMAX_SYSCALL_INTERRUPT_PRIORITY);
#if defined(AM_PART_APOLLO4)
    #if defined(COOPER_QFN)
        NVIC_SetPriority(GPIO0_001F_IRQn, NVIC_configMAX_SYSCALL_INTERRUPT_PRIORITY);
    #else
        NVIC_SetPriority(GPIO0_203F_IRQn, NVIC_configMAX_SYSCALL_INTERRUPT_PRIORITY);
    #endif
#else
    NVIC_SetPriority(GPIO_IRQn, NVIC_configMAX_SYSCALL_INTERRUPT_PRIORITY);
#endif

}

//*****************************************************************************
//
// Short Description.
//
//*****************************************************************************
void
RadioTask(void *pvParameters)
{
#if WSF_TRACE_ENABLED == TRUE
    //
    // Enable ITM
    //
    am_util_debug_printf("Starting wicentric trace:\n\n");
#endif

    //
    // Boot the radio.
    //
    HciDrvRadioBoot(1);

    //
    // Initialize the main ExactLE stack.
    //
    exactle_stack_init();

    // uncomment the following to set custom Bluetooth address here
    // {
    //     uint8_t bd_addr[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    //     HciVscSetCustom_BDAddr(&bd_addr);
    // }

    //
    // Start the "Amdtp" profile.
    //
    AmdtpcStart();

    while (1)
    {
        //
        // Calculate the elapsed time from our free-running timer, and update
        // the software timers in the WSF scheduler.
        //
        wsfOsDispatcher();

    }
}
