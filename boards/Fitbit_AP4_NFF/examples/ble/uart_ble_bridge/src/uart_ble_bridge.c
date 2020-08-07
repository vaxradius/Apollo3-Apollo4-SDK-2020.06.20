//*****************************************************************************
//
//! @file uart_ble_bridge.c
//!
//! @brief Converts UART HCI commands to SPI.
//!
//! This exapmle can be used as a way to communicate between a host chip using
//! UART HCI and the BLE module inside Apollo3.
//
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

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"
#include "am_util_ble_cooper.h"
#include "am_devices_cooper.h"
#include "wsf_types.h"

//*****************************************************************************
//
// Configuration options
//
//*****************************************************************************
//
// Define the UART module (0 or 1) to be used.
// Also define the max packet size
//
#define UART_HCI_BRIDGE                 0
#define MAX_UART_PACKET_SIZE            2048

//*****************************************************************************
//
// SPI configuration.
//
//*****************************************************************************
#define COOPER_IOM_FREQ         AM_HAL_IOM_24MHZ

#if defined(COOPER_QFN)
#define SPI_MODULE           0
#define AM_COOPER_IRQn       GPIO0_001F_IRQn
#define am_cooper_irq_isr    am_gpio0_001f_isr
#else
#define SPI_MODULE           4
#define AM_COOPER_IRQn       GPIO0_203F_IRQn
#define am_cooper_irq_isr    am_gpio0_203f_isr
#endif


//*****************************************************************************
//
// Custom data type.
// Note - am_uart_buffer was simply derived from the am_hal_iom_buffer macro.
//
//*****************************************************************************
#define am_uart_buffer(A)                                                   \
union                                                                   \
  {                                                                       \
    uint32_t words[(A + 3) >> 2];                                       \
      uint8_t bytes[A];                                                   \
  }

//*****************************************************************************
//
// Global Variables
//
//*****************************************************************************
// Buffer for non-blocking transactions
#if defined(USE_NONBLOCKING_HCI)
AM_SHARED_RW uint32_t DMATCBBuffer[2048];
AM_SHARED_RW uint8_t g_pui8UARTTXBuffer[MAX_UART_PACKET_SIZE];
AM_SHARED_RW am_uart_buffer(1024) g_psWriteData;
AM_SHARED_RW am_uart_buffer(1024) g_psReadData;
#else
uint32_t DMATCBBuffer[2048];
uint8_t g_pui8UARTTXBuffer[MAX_UART_PACKET_SIZE];
am_uart_buffer(1024) g_psWriteData;
am_uart_buffer(1024) g_psReadData;
#endif

volatile uint32_t g_ui32UARTRxIndex = 0;
volatile bool g_bRxTimeoutFlag = false;
volatile bool g_bCmdProcessedFlag = false;

// 01 C0 FC 06 20 00 99 88 77 66
uint8_t hci_vsc_write_info0_signature_cmd[1+2+1+4] = {0x01, 0x75, 0xFC, 0x04, 0xC9, 0x18, 0x5D, 0xB3};
uint8_t store_info0_trim_virtual_cmd[1+2+1+2+4] = {0x01, 0xC0, 0xFC, 0x6, 04, 00, 0x99, 0x88, 0x77, 066};
uint8_t store_info0_trim_virtual_cmd_complete[1+1+1+1+2+1] = {0x04, 0x0e, 0x04, 0x05, 0xC0, 0xFC, 0x00};
// 01 C1 FC 00
uint8_t trigger_flash_info0_virtual_cmd[1+2+1] = {0x01, 0xC1, 0xFC, 00};
uint8_t trigger_flash_info0_virtual_cmd_complete[1+1+1+1+2+1] = {0x04, 0x0e, 0x04, 0x05, 0xC1, 0xFC, 0x00};

uint32_t                   info0_trimmedData_index = 0;
am_sbl_info0_patch_data_t  info0_trimmedData[AM_DEVICES_COOPER_SBL_MAX_INFO_0_PATCH_VALUES] = {0};

am_sbl_info0_patch_blob_t  info0dataBuf;

#define RUNNING_GPIO_PIN    (31)
#define DONE_GPIO_PIN       (32)

//*****************************************************************************
//
// Process "special" UART commands.  Format is:
//      'A'     Header
//      'M'
//      'Y'     Command (ASCII '0' - '2')
//       X      Value   (0 - 255)
//       X
//
//*****************************************************************************
void *g_IomDevHdl;
void *g_pvHciSpiHandle;
void *g_pvUART;

#define IOM_INTERRUPT1(n)       AM_HAL_INTERRUPT_IOMASTER ## n
#define IOM_INTERRUPT(n)        IOM_INTERRUPT1(n)
#define COOPER_IOM_IRQn           ((IRQn_Type)(IOMSTR0_IRQn + SPI_MODULE))

//
// Take over the interrupt handler for whichever IOM we're using.
//
#define cooper_iom_isr                                                          \
    am_iom_isr1(SPI_MODULE)
#define am_iom_isr1(n)                                                        \
    am_iom_isr(n)
#define am_iom_isr(n)                                                         \
    am_iomaster ## n ## _isr

//*****************************************************************************
//
// IOM ISRs.
//
//*****************************************************************************
//
//! Take over default ISR. (Queue mode service)
//
void cooper_iom_isr(void)
{
    uint32_t ui32Status;

    if (!am_hal_iom_interrupt_status_get(g_pvHciSpiHandle, true, &ui32Status))
    {
        if ( ui32Status )
        {
            am_hal_iom_interrupt_clear(g_pvHciSpiHandle, ui32Status);
            am_hal_iom_interrupt_service(g_pvHciSpiHandle, ui32Status);
        }
    }
}

#if 0
uint8_t carrier_wave_mode = 0;

static void cmd_handler(uint8_t *pBuffer, uint32_t len)
{
    uint16_t value;

#ifndef AM_DEBUG_BLE_TIMING
    for ( uint32_t i = 0; i < len; i++ )
    {
        am_util_stdio_printf("%02x ", pBuffer[i]);
    }
    am_util_stdio_printf("\n");
#endif

    carrier_wave_mode = 0;

    if ( (NULL != pBuffer) && (pBuffer[0] == 0x01) && (pBuffer[1] == 0x1e) && (pBuffer[2] == 0x20) )
    {
        switch(pBuffer[6])
        {
            case 0x09:      // constant transmission mode
            {
                am_util_ble_set_constant_transmission(g_IomDevHdl, true);  // set constant transmission
                pBuffer[6] = 0x00;
            }
            break;

            case 0x08:      // carrier wave mode
            {
                carrier_wave_mode = 1;  //set  carrier wave mode
                pBuffer[6] = 0x00;
            }
            break;

             /*added a new command for stopping carrier wave mode transmitting if needed*/
            case 0xff:
            {
                 am_util_ble_transmitter_control(g_IomDevHdl, 0);           //disable carrier_wave_mode
                 am_util_ble_set_constant_transmission(g_IomDevHdl, false); //disable constant transmission mode
            }
            break;

            default:
            break;
        }
    }

    //
    // Check the parameters and the UART command format.
    //
    if ( (NULL != pBuffer) && (len == 5) && (pBuffer[0] == 'A') && (pBuffer[1] == 'M') )
    {
        //
        // Compute the value.
        //
        value = pBuffer[3] << 8 | pBuffer[4];

        //
        // Interpret the Command 'Y'
        //
        switch (pBuffer[2])
        {
            case '0':   // handle the tx power setting command.
            {
                // Check the TX power range value.
                if ((value > 0) && (value <= 0xF))
                {
                    am_devices_cooper_tx_power_set(g_IomDevHdl, (uint8_t)value);
#ifndef AM_DEBUG_BLE_TIMING
                    am_util_stdio_printf("TX Power Setting Command OK\n");
#endif
                }
                else
                {
#ifndef AM_DEBUG_BLE_TIMING
                    am_util_stdio_printf("Invalid TX Power Value %d\n", value);
#endif
                }
            }
            break;

            case '1':   // handle the 32MHz crystal trim setting command.
            {
                am_util_ble_crystal_trim_set(g_IomDevHdl, value);
#ifndef AM_DEBUG_BLE_TIMING
                am_util_stdio_printf("32MHz Crystal Trim Command OK\n");
#endif
            }
            break;

            case '2':   // handle modulation index setting command.
            {
                am_util_ble_transmitter_modex_set(g_IomDevHdl, (uint8_t)value);
#ifndef AM_DEBUG_BLE_TIMING
                am_util_stdio_printf("Modulation Index Command OK\n");
#endif
            }
            break;

            case '3':  // handle the carrier wave output command
            {
                am_util_ble_transmitter_control_ex(g_IomDevHdl, (uint8_t)value);
#ifndef AM_DEBUG_BLE_TIMING
                am_util_stdio_printf("generate carrier wave OK\n");
#endif
            }
            break;

            case '4':  // handle the continually transmitting output command
            {
                am_util_ble_set_constant_transmission_ex(g_IomDevHdl, (uint8_t)value);
#ifndef AM_DEBUG_BLE_TIMING
                am_util_stdio_printf("generate constant moderated signal wave OK\n");
#endif
            }
            break;

            default:
            {
#ifndef AM_DEBUG_BLE_TIMING
                am_util_stdio_printf("Invalid UART Special Command %s\r\n", pBuffer);
#endif
            }
            break;
        }
        g_bCmdProcessedFlag = true;
    }
    else
    {
        g_bCmdProcessedFlag = false;
    }
}
#endif

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

//*****************************************************************************
//
// Interrupt handler for the UART.
//
//*****************************************************************************
#if UART_HCI_BRIDGE == 0
void am_uart_isr(void)
#else
void am_uart1_isr(void)
#endif
{
    uint32_t ui32Status;

    //
    // Read the masked interrupt status from the UART.
    //
    am_hal_uart_interrupt_status_get(g_pvUART, &ui32Status, true);
    am_hal_uart_interrupt_clear(g_pvUART, ui32Status);
    am_hal_uart_interrupt_service(g_pvUART, ui32Status);
    //
    // If there's an RX interrupt, handle it in a way that preserves the
    // timeout interrupt on gaps between packets.
    //
    if (ui32Status & (AM_HAL_UART_INT_RX_TMOUT | AM_HAL_UART_INT_RX))
    {
        uint32_t ui32BytesRead;

        am_hal_uart_transfer_t sRead =
        {
            .eType = AM_HAL_UART_BLOCKING_READ,
            .ui32TimeoutMs = 1000,
            .pui8Data = (uint8_t *) &(g_psWriteData.bytes[g_ui32UARTRxIndex]),
            .ui32NumBytes = 23,
            .pui32BytesTransferred = &ui32BytesRead,
        };

        am_hal_uart_transfer(g_pvUART, &sRead);

        g_ui32UARTRxIndex += ui32BytesRead;

        //
        // If there is a TMOUT interrupt, assume we have a compete packet, and
        // send it over SPI.
        //
        if (ui32Status & (AM_HAL_UART_INT_RX_TMOUT))
        {
            NVIC_DisableIRQ((IRQn_Type)(UART0_IRQn + UART_HCI_BRIDGE));
            if(ui32BytesRead)
            {
                //cmd_handler(g_psWriteData.bytes, g_ui32UARTRxIndex);
                g_bRxTimeoutFlag = true;
            }
        }
    }
}

void running_done_gpio_init(void)
{
    // Running GPIO init
    am_hal_gpio_pinconfig(RUNNING_GPIO_PIN, am_hal_gpio_pincfg_output);
    am_hal_gpio_state_write(RUNNING_GPIO_PIN, AM_HAL_GPIO_OUTPUT_CLEAR);

    // Done GPIO init
    am_hal_gpio_pinconfig(DONE_GPIO_PIN, am_hal_gpio_pincfg_output);
    am_hal_gpio_state_write(DONE_GPIO_PIN, AM_HAL_GPIO_OUTPUT_CLEAR);
}

void running_done_gpio_set(bool is_running, bool is_done)
{
    am_hal_gpio_state_write(RUNNING_GPIO_PIN, (am_hal_gpio_write_type_e)is_running);
    am_hal_gpio_state_write(DONE_GPIO_PIN, (am_hal_gpio_write_type_e)is_done);
}

uint32_t reset_with_sbl_check(am_devices_cooper_config_t* pDevConfig)
{
    uint32_t sbl_status = 0;
    am_devices_cooper_reset();
    am_devices_cooper_image_update_init(g_IomDevHdl, pDevConfig->pNBTxnBuf);
    sbl_status = AM_DEVICES_COOPER_SBL_STATUS_INIT;
    sbl_status = am_devices_cooper_update_image();
    while( (sbl_status != AM_DEVICES_COOPER_SBL_STATUS_OK) && ( sbl_status != AM_DEVICES_COOPER_SBL_STATUS_FAIL) )
    {
        while (am_devices_cooper_irq_read(g_IomDevHdl) == 0)
        {
            am_hal_delay_us(50);
        }
        sbl_status = am_devices_cooper_update_image();
    }
    //
    // Return the status.
    //
    if (sbl_status == AM_DEVICES_COOPER_SBL_STATUS_OK)
    {
        // need to wait a bit to jump from SBL to Cooper application firmware
        am_util_delay_ms(10);
        am_util_stdio_printf("SBL Done\r\n");
        return AM_DEVICES_COOPER_STATUS_SUCCESS;
    }
    else
    {
        // free up resource that won't be used.
        am_devices_cooper_term(g_IomDevHdl);
        g_IomDevHdl = NULL;
        g_pvHciSpiHandle = NULL;
        am_util_stdio_printf("SBL Cooper Error 0x%x\r\n", sbl_status);
        return AM_DEVICES_COOPER_STATUS_ERROR;
    }
}

//*****************************************************************************
//
// Main
//
//*****************************************************************************
int
main(void)
{
    uint32_t ui32NumChars;
    uint32_t ui32Status;
    uint32_t ui32IntStatus;
    uint32_t ui32BytesTransferred = 0;

    //
    // Default setup.
    //
    am_bsp_low_power_init();

    //
    // Enable the ITM
    //
    am_bsp_itm_printf_enable();
    am_util_stdio_printf("Apollo4 UART to SPI Bridge\n");

    am_devices_cooper_config_t stCooperConfig;
    stCooperConfig.ui32ClockFreq = COOPER_IOM_FREQ;
    stCooperConfig.pNBTxnBuf = DMATCBBuffer;
    stCooperConfig.ui32NBTxnBufLength = sizeof(DMATCBBuffer) / 4;

    running_done_gpio_init();

    running_done_gpio_set(1, 0);
    //
    // Initialize the SPI module.
    //
    ui32Status = am_devices_cooper_init(SPI_MODULE, &stCooperConfig, &g_IomDevHdl, &g_pvHciSpiHandle);
    if(ui32Status)
    {
        running_done_gpio_set(0, 0);
        return -1;
    }

    am_hal_iom_interrupt_clear(g_pvHciSpiHandle, AM_HAL_IOM_INT_DCMP | AM_HAL_IOM_INT_CMDCMP);
    am_hal_iom_interrupt_enable(g_pvHciSpiHandle, AM_HAL_IOM_INT_DCMP | AM_HAL_IOM_INT_CMDCMP);
    NVIC_EnableIRQ(COOPER_IOM_IRQn);

    //
    // Start the UART.
    //
    am_hal_uart_config_t sUartConfig =
    {
        //
        // Standard UART settings: 115200-8-N-1
        //
        .ui32BaudRate    = 115200,
        //
        // Set TX and RX FIFOs to interrupt at three-quarters full.
        //
        .eDataBits    = AM_HAL_UART_DATA_BITS_8,
        .eParity      = AM_HAL_UART_PARITY_NONE,
        .eStopBits    = AM_HAL_UART_ONE_STOP_BIT,
        .eFlowControl = AM_HAL_UART_FLOW_CTRL_NONE,
        .eTXFifoLevel = AM_HAL_UART_FIFO_LEVEL_28,
        .eRXFifoLevel = AM_HAL_UART_FIFO_LEVEL_28,
    };

    am_hal_uart_initialize(UART_HCI_BRIDGE, &g_pvUART);
    am_hal_uart_power_control(g_pvUART, AM_HAL_SYSCTRL_WAKE, false);
    am_hal_uart_configure(g_pvUART, &sUartConfig);
    am_hal_uart_buffer_configure(g_pvUART, g_pui8UARTTXBuffer, sizeof(g_pui8UARTTXBuffer), NULL, 0);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_COM_UART_TX, g_AM_BSP_GPIO_COM_UART_TX);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_COM_UART_RX, g_AM_BSP_GPIO_COM_UART_RX);
    //
    // Make sure to enable the interrupts for RX, since the HAL doesn't already
    // know we intend to use them.
    //
    NVIC_EnableIRQ((IRQn_Type)(UART0_IRQn + UART_HCI_BRIDGE));
    am_hal_uart_interrupt_enable(g_pvUART, (AM_HAL_UART_INT_RX |
                                 AM_HAL_UART_INT_RX_TMOUT));

    am_hal_interrupt_master_enable();

    // send hci_reset
    am_util_ble_hci_reset(g_IomDevHdl);
    am_util_delay_ms(1);

    running_done_gpio_set(0, 1);

    //
    // Loop forever.
    //
    while (1)
    {
        //
        // Check for incoming traffic from either the UART or the BLE interface.
        //
        ui32IntStatus = am_devices_cooper_irq_read(g_IomDevHdl);

        if ( ui32IntStatus > 0 )
        {
            //
            // If we have incoming BLE traffic, read it into a buffer.
            //
            ui32Status = am_devices_cooper_blocking_read(g_IomDevHdl,
                                                      g_psReadData.words,
                                                      &ui32NumChars);

            //
            // If the read was successful, echo it back out over the UART.
            //
            if ( ui32Status == AM_DEVICES_COOPER_STATUS_SUCCESS )
            {
                if (ui32NumChars > 0)
                {
                    ui32BytesTransferred = 0;
                    am_hal_uart_transfer_t sWrite =
                    {
                        .eType = AM_HAL_UART_BLOCKING_WRITE,
                        .pui8Data = g_psReadData.bytes,
                        .ui32NumBytes = ui32NumChars,
                        .ui32TimeoutMs = AM_HAL_UART_WAIT_FOREVER,
                        .pui32BytesTransferred = &ui32BytesTransferred,
                    };

                    //please enable here for transmission mode set
                    //fix_trans_mode(g_psReadData.bytes);

                    //then send the response to UART
                    am_hal_uart_transfer(g_pvUART, &sWrite);
                    am_util_delay_ms(1);
                }
            }

            else
            {
                //
                // Handle the error here.
                //
                am_util_stdio_printf("Read from BLE Controller failed\n");
                while(1);
            }
        }
        else if (g_bRxTimeoutFlag)
        {
            //
            // If we have incoming UART traffic, the interrupt handler will
            // read it out for us, but we will need to echo it back out to the
            // radio manually.
            //
            if (false == g_bCmdProcessedFlag)
            {
                if (memcmp(g_psWriteData.words, store_info0_trim_virtual_cmd, (uint32_t)3) == 0)
                {
                    // store info0 trimmed data
                    uint32_t wordOffset, trimmedValue;

                    wordOffset = g_psWriteData.bytes[4] + (g_psWriteData.bytes[5] << 8);
                    wordOffset = wordOffset>>2;

                    trimmedValue = g_psWriteData.bytes[6] + (g_psWriteData.bytes[7] << 8);
                    trimmedValue += (g_psWriteData.bytes[8] << 16) + (g_psWriteData.bytes[9] << 24);

                    info0_trimmedData[info0_trimmedData_index].wordOffset = wordOffset;
                    info0_trimmedData[info0_trimmedData_index].value      = trimmedValue;

                    info0_trimmedData_index++;

                    am_util_stdio_printf("wordOffset =0x%x, trimmedvalue = 0x%0x\r\n", wordOffset, trimmedValue);

                    // send hci command complete back.

                    // copy first
                    memcpy(g_psReadData.bytes, store_info0_trim_virtual_cmd_complete, sizeof(store_info0_trim_virtual_cmd_complete));
                    ui32NumChars = sizeof(store_info0_trim_virtual_cmd_complete);

                    ui32BytesTransferred = 0;
                    am_hal_uart_transfer_t sWrite =
                    {
                        .eType = AM_HAL_UART_BLOCKING_WRITE,
                        .pui8Data = g_psReadData.bytes,
                        .ui32NumBytes = ui32NumChars,
                        .ui32TimeoutMs = AM_HAL_UART_WAIT_FOREVER,
                        .pui32BytesTransferred = &ui32BytesTransferred,
                    };

                    //then send the response to UART
                    am_hal_uart_transfer(g_pvUART, &sWrite);
                    am_util_delay_ms(1);

                }
                else if (memcmp(g_psWriteData.words, trigger_flash_info0_virtual_cmd, (uint32_t)3) == 0)
                {
                    // call SBL API to update info0 into its internal structure

                    // Need to review with Deepak.
                    if (info0_trimmedData_index > 0)
                    {
                        running_done_gpio_set(1, 0);

                        // supply info0 patch to SBL.
                        am_devices_cooper_get_info0_patch(&info0dataBuf, info0_trimmedData_index, info0_trimmedData);

                        // reset variables related to info0
                        info0_trimmedData_index = 0;
                        memset(info0_trimmedData, 0, sizeof(info0_trimmedData));

                        // write HCI command to trigger Cooper to reboot for SBL to do download.
                        am_devices_cooper_blocking_write(g_IomDevHdl, AM_DEVICES_COOPER_RAW,
                                                (uint32_t*)&hci_vsc_write_info0_signature_cmd,
                                                sizeof(hci_vsc_write_info0_signature_cmd));

                        // ignore event for the above HCI VSC.

                        // wait for finish writing
                        am_util_delay_ms(100);

                        // reset Cooper to get SBL to update inf0
                        am_util_stdio_printf("Reset Cooper for info0 updating\r\n");
                        ui32Status = reset_with_sbl_check(&stCooperConfig);
                        if(ui32Status)
                        {
                            am_util_stdio_printf("am_devices_cooper_init fails on info0 update\r\n");
                            running_done_gpio_set(0, 0);
                            return -1;
                        }

                        // reset Cooper for inf0 to take effect
                        am_util_stdio_printf("Reset Cooper for info0 taking effective\r\n");
                        ui32Status = reset_with_sbl_check(&stCooperConfig);
                        if(ui32Status)
                        {
                            am_util_stdio_printf("am_devices_cooper_init fails after info0 update\r\n");
                            running_done_gpio_set(0, 0);
                            return -1;
                        }
                        // send hci_reset
                        am_util_ble_hci_reset(g_IomDevHdl);
                        am_util_delay_ms(1);
                        running_done_gpio_set(0, 1);
                    }

                    // send hci command complete back for trigger_flash_info0_virtual_cmd.

                    // copy first
                    memcpy(g_psReadData.bytes, trigger_flash_info0_virtual_cmd_complete, sizeof(trigger_flash_info0_virtual_cmd_complete));
                    ui32NumChars = sizeof(trigger_flash_info0_virtual_cmd_complete);

                    ui32BytesTransferred = 0;
                    am_hal_uart_transfer_t sWrite =
                    {
                        .eType = AM_HAL_UART_BLOCKING_WRITE,
                        .pui8Data = g_psReadData.bytes,
                        .ui32NumBytes = ui32NumChars,
                        .ui32TimeoutMs = AM_HAL_UART_WAIT_FOREVER,
                        .pui32BytesTransferred = &ui32BytesTransferred,
                    };

                    //then send the response to UART
                    am_hal_uart_transfer(g_pvUART, &sWrite);
                    am_util_delay_ms(1);
                }
                else {
                    // go to Cooper over HCI interface.
                    am_devices_cooper_blocking_write(g_IomDevHdl, AM_DEVICES_COOPER_RAW,
                                              g_psWriteData.words,
                                              g_ui32UARTRxIndex);
                }
            }

            g_ui32UARTRxIndex = 0;
            g_bRxTimeoutFlag = false;
            NVIC_EnableIRQ((IRQn_Type)(UART0_IRQn + UART_HCI_BRIDGE));
        }
    }
}
