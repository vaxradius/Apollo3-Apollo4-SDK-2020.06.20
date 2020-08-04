//*****************************************************************************
//
//! @file am_util_ble_cooper.c
//!
//! @brief Useful BLE functions not covered by the HAL.
//!
//! This file contains functions for interacting with the Apollo4 BLE hardware
//! that are not already covered by the HAL. Most of these commands either
//! adjust RF settings or facilitate RF testing operations.
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
#include <stdbool.h>
#include <string.h>
#include "am_util_delay.h"
#include "am_mcu_apollo.h"
#include "am_devices_cooper.h"

//*****************************************************************************
//
// Globals
//
//*****************************************************************************

//*****************************************************************************
//
// In DTM mode, set TX to constant trans mode for SRRC/FCC/CE
//set enable as 'true' to constant trans mode, 'false' back to normal
//*****************************************************************************
uint32_t
am_util_ble_set_constant_transmission(void *pHandle, bool enable)
{
    //TODO
#if 0
    am_devices_cooper_t *pBLE = (am_devices_cooper_t *)pHandle;

    am_devices_cooper_sleep_set(pHandle, false);
    am_devices_cooper_plf_reg_write(pBLE, 0x43000004, 0xFFFFFFFF);
    if ( enable )
    {
        am_devices_cooper_plf_reg_write(pBLE, 0x508000E0, 0x00008000);
    }
    else
    {
        am_devices_cooper_plf_reg_write(pBLE, 0x508000E0, 0x00000000);
    }
#endif
    return AM_DEVICES_COOPER_STATUS_SUCCESS;
}

//*****************************************************************************
//
// Manually enable/disable transmitter
// set ui8TxCtrl as 1 to manually enable transmitter, 0 back to default
//
//*****************************************************************************
uint32_t
am_util_ble_transmitter_control(void *pHandle, uint8_t ui8TxCtrl)
{
    //TODO
#if 0
    am_devices_cooper_t *pBLE = (am_devices_cooper_t *)pHandle;
    uint32_t RegValueTRX;

    am_devices_cooper_sleep_set(pHandle, false);
    if (ui8TxCtrl)
    {
        RegValueTRX = 0x2000A;
    }
    else
    {
        RegValueTRX = 0x8;
    }

    //
    // Unlock the BLE registers.
    //
    am_devices_cooper_plf_reg_write(pBLE, 0x43000004, 0xFFFFFFFF);
    am_devices_cooper_plf_reg_write(pBLE, 0x52400000, RegValueTRX);
#endif
    return AM_DEVICES_COOPER_STATUS_SUCCESS;
}

//*****************************************************************************
//
//to fix the channel 1 bug in DTM mode
//
//*****************************************************************************

uint32_t
am_util_ble_init_rf_channel(void *pHandle)
{
#if ((defined(AM_PART_APOLLO4)) && (defined(USE_NONBLOCKING_HCI)))
    AM_SHARED_RW am_devices_cooper_buffer(16) sWriteCommand;
    AM_SHARED_RW am_devices_cooper_buffer(16) sResponse;
#else
    am_devices_cooper_buffer(16) sWriteCommand;
    am_devices_cooper_buffer(16) sResponse;
#endif
    uint32_t ui32Status;
    uint8_t ui8Channel = 0;

    am_devices_cooper_sleep_set(pHandle, false);
    am_util_delay_ms(1);

    //issue the HCI command with to init for the channel 1
    ui32Status = am_devices_cooper_command_write(pHandle, sWriteCommand.words, sResponse.words, 0x201D, 5, &ui8Channel);
    if ( ui32Status != AM_DEVICES_COOPER_STATUS_SUCCESS )
    {
        return ui32Status;
    }
    am_util_delay_ms(10);

    // issue the HCI command with to stop test for the channel 1
    ui32Status = am_devices_cooper_command_write(pHandle, sWriteCommand.words, sResponse.words, 0x201F, 4, NULL);
    if ( ui32Status != AM_DEVICES_COOPER_STATUS_SUCCESS )
    {
        return ui32Status;
    }
    return AM_DEVICES_COOPER_STATUS_SUCCESS;
}

//*****************************************************************************
//
// BLE init for BQB test
//set enable as 'true' to init as BQB test mode, 'false' back to default
//*****************************************************************************
uint32_t
am_util_ble_BQB_test_init(void *pHandle, bool enable)
{
    //TODO
#if 0
    am_devices_cooper_t *pBLE = (am_devices_cooper_t *)pHandle;

    am_devices_cooper_sleep_set(pHandle, false);
    am_devices_cooper_plf_reg_write(pBLE, 0x43000004, 0xFFFFFFFF);

    if ( enable )
    {
        am_devices_cooper_plf_reg_write(pBLE, 0x51800028, 0x0000209c);
    }
    else
    {
        am_devices_cooper_plf_reg_write(pBLE, 0x51800028, 0x00003ff6);
    }

    am_devices_cooper_plf_reg_write(pBLE, 0x45800070, 0x100);
    am_devices_cooper_plf_reg_write(pBLE, 0x45800070, 0);
#endif
    return AM_DEVICES_COOPER_STATUS_SUCCESS;
}

//*****************************************************************************
//
// Set the 32M crystal frequency
// based on the tested values at customer side.
// set trim value smaller in case of negative frequency offset
// ui32TrimValue: default is 0x400
//*****************************************************************************
uint32_t
am_util_ble_crystal_trim_set(void *pHandle, uint32_t ui32TrimValue)
{
    //TODO
#if 0
    am_devices_cooper_t *pBLE = (am_devices_cooper_t *)pHandle;
    uint32_t RegValueMCGR;

    ui32TrimValue &= 0x7FF;

    am_devices_cooper_plf_reg_read(pBLE, 0x43000004, &RegValueMCGR);
    //
    // Unlock the BLE registers.
    //
    am_devices_cooper_plf_reg_write(pBLE, 0x43000004, 0xFFFFFFFF);
    am_devices_cooper_plf_reg_write(pBLE, 0x43800004, ui32TrimValue);
    am_devices_cooper_plf_reg_write(pBLE, 0x43000004, RegValueMCGR);
#endif
    return AM_DEVICES_COOPER_STATUS_SUCCESS;
}

//*****************************************************************************
//
// Manually enable/disable transmitter to output carrier signal
// set ui8TxChannel as 0 to 0x27 for each transmit channel, 0xFF back to normal modulate mode
//
//*****************************************************************************
uint32_t
am_util_ble_hci_reset(void *pHandle)
{
#if ((defined(AM_PART_APOLLO4)) && (defined(USE_NONBLOCKING_HCI)))
    AM_SHARED_RW am_devices_cooper_buffer(16) sWriteCommand;
    AM_SHARED_RW am_devices_cooper_buffer(16) sResponse;
#else
    am_devices_cooper_buffer(16) sWriteCommand;
    am_devices_cooper_buffer(16) sResponse;
#endif
    uint32_t ui32Status;

    am_devices_cooper_sleep_set(pHandle, false);
    am_util_delay_ms(1);

    // issue the HCI command with to reset hci
    ui32Status = am_devices_cooper_command_write(pHandle, sWriteCommand.words, sResponse.words, 0x0C03, 4, NULL);
    if ( ui32Status != AM_DEVICES_COOPER_STATUS_SUCCESS )
    {
        return ui32Status;
    }
    return AM_DEVICES_COOPER_STATUS_SUCCESS;
}

//*****************************************************************************
//
//to do directly output modulation signal. change channel ranges from 0 to 0x27, pattern from 0 to 7.
//
//*****************************************************************************
uint32_t
am_util_ble_trasmitter_test_ex(void *pHandle, uint8_t channel, uint8_t pattern)
{
#if ((defined(AM_PART_APOLLO4)) && (defined(USE_NONBLOCKING_HCI)))
    AM_SHARED_RW am_devices_cooper_buffer(16) sWriteCommand;
    AM_SHARED_RW am_devices_cooper_buffer(16) sResponse;
#else
    am_devices_cooper_buffer(16) sWriteCommand;
    am_devices_cooper_buffer(16) sResponse;
#endif
    uint32_t ui32Status;
    uint8_t pui8Parameter[3] = {channel, 0x25, pattern};

    am_devices_cooper_sleep_set(pHandle, false);
    am_util_delay_ms(1);

    // issue the HCI command with to TX carrier wave
    ui32Status = am_devices_cooper_command_write(pHandle, sWriteCommand.words, sResponse.words, 0x201E, 7, pui8Parameter);
    if ( ui32Status != AM_DEVICES_COOPER_STATUS_SUCCESS )
    {
        return ui32Status;
    }
    return AM_DEVICES_COOPER_STATUS_SUCCESS;
}

//*****************************************************************************
//
//to do directly receiver test. change channel ranges from 0 to 0x27, return received packets in 100ms.
//
//*****************************************************************************

uint32_t
am_util_ble_receiver_test_ex(void *pHandle, uint8_t channel, uint32_t *recvpackets)
{
#if ((defined(AM_PART_APOLLO4)) && (defined(USE_NONBLOCKING_HCI)))
    AM_SHARED_RW am_devices_cooper_buffer(16) sWriteCommand;
    AM_SHARED_RW am_devices_cooper_buffer(16) sResponse;
#else
    am_devices_cooper_buffer(16) sWriteCommand;
    am_devices_cooper_buffer(16) sResponse;
#endif
    uint32_t ui32Status;

    am_devices_cooper_sleep_set(pHandle, false);
    am_util_delay_ms(1);

    ui32Status = am_devices_cooper_command_write(pHandle, sWriteCommand.words, sResponse.words, 0x201D, 5, &channel);
    if ( ui32Status != AM_DEVICES_COOPER_STATUS_SUCCESS )
    {
        return ui32Status;
    }

    am_util_delay_ms(100);

    // issue the HCI command with to stop test for the channel 1
    ui32Status = am_devices_cooper_command_write(pHandle, sWriteCommand.words, sResponse.words, 0x201F, 4, NULL);
    if ( ui32Status != AM_DEVICES_COOPER_STATUS_SUCCESS )
    {
        return ui32Status;
    }
    *recvpackets = (sResponse.bytes[8] << 8) + sResponse.bytes[7];

    return AM_DEVICES_COOPER_STATUS_SUCCESS;
}

//*****************************************************************************
//
//to directly output carrier wave. change channel ranges from 0 to 0x27.
//
//*****************************************************************************
uint32_t
am_util_ble_set_carrier_wave_ex(void *pHandle, uint8_t channel)
{
#if ((defined(AM_PART_APOLLO4)) && (defined(USE_NONBLOCKING_HCI)))
    AM_SHARED_RW am_devices_cooper_buffer(16) sWriteCommand;
    AM_SHARED_RW am_devices_cooper_buffer(16) sResponse;
#else
    am_devices_cooper_buffer(16) sWriteCommand;
    am_devices_cooper_buffer(16) sResponse;
#endif
    uint32_t ui32Status;
    uint8_t pui8Parameter[3] = {channel, 0x25, 0};

    // channel 0xFF to disable the constant transmission
    if ( channel == 0xFF )
    {
        am_util_ble_transmitter_control(pHandle, false);
        return AM_DEVICES_COOPER_STATUS_SUCCESS;
    }

    am_devices_cooper_sleep_set(pHandle, false);
    am_util_delay_ms(1);

    // issue the HCI command with to TX carrier wave
    ui32Status = am_devices_cooper_command_write(pHandle, sWriteCommand.words, sResponse.words, 0x201E, 7, pui8Parameter);
    if ( ui32Status != AM_DEVICES_COOPER_STATUS_SUCCESS )
    {
        return ui32Status;
    }

    am_util_ble_transmitter_control(pHandle, true);

    return AM_DEVICES_COOPER_STATUS_SUCCESS;
}

//*****************************************************************************
//
// Manually enable/disable transmitter to output carrier wave signal
// set ui8TxChannel as 0 to 0x27 for each transmit channel, 0xFF back to normal modulate mode
//
//*****************************************************************************
uint32_t
am_util_ble_transmitter_control_ex(void *pHandle, uint8_t ui8TxChannel)
{
    return am_util_ble_set_carrier_wave_ex(pHandle, ui8TxChannel);
}

//*****************************************************************************
//
//to directly output constant modulation signal. change channel from 0 to 0x27.
//
//*****************************************************************************
uint32_t
am_util_ble_set_constant_transmission_ex(void *pHandle, uint8_t channel)
{
#if ((defined(AM_PART_APOLLO4)) && (defined(USE_NONBLOCKING_HCI)))
    AM_SHARED_RW am_devices_cooper_buffer(16) sWriteCommand;
    AM_SHARED_RW am_devices_cooper_buffer(16) sResponse;
#else
    am_devices_cooper_buffer(16) sWriteCommand;
    am_devices_cooper_buffer(16) sResponse;
#endif
    uint32_t ui32Status;
    uint8_t pui8Parameter[3] = {channel, 0x25, 0};

    // channel 0xFF to disable the constant transmission
    if ( channel == 0xFF )
    {
        am_util_ble_set_constant_transmission(pHandle, false);
        return AM_DEVICES_COOPER_STATUS_SUCCESS;
    }
    am_util_ble_set_constant_transmission(pHandle, true);

    // issue the HCI command with to TX constant transmission
    ui32Status = am_devices_cooper_command_write(pHandle, sWriteCommand.words, sResponse.words, 0x201E, 7, pui8Parameter);
    if ( ui32Status != AM_DEVICES_COOPER_STATUS_SUCCESS )
    {
        return ui32Status;
    }
    return AM_DEVICES_COOPER_STATUS_SUCCESS;
}

//*****************************************************************************
//
// read current modex value from BLEIP
//*****************************************************************************
uint32_t
am_util_ble_read_modex_value(void *pHandle)
{
    uint32_t temp = 0;
    //TODO
#if 0
    if (APOLLO3_GE_B0)
    {
        // for B0 Chip,the modex value address is changed to 0x20006874
        am_devices_cooper_plf_reg_read(pBLE, 0x20006874, &temp);
    }
    else
    {
        am_devices_cooper_plf_reg_read(pBLE, 0x20006070, &temp);
    }
#endif
    return temp;
}

//*****************************************************************************
//
// Manually set modulation characteristic
// based on the tested values at customer side.
// manually set frequency offset for 10101010 or 01010101 pattern
// parameter default value is 0x34, increase to get larger frequency offset
//
//*****************************************************************************
uint32_t
am_util_ble_transmitter_modex_set(void *pHandle, uint8_t ui8ModFrqOffset)
{
    //TODO
#if 0
    am_devices_cooper_t *pBLE = (am_devices_cooper_t *)pHandle;
    uint32_t RegValueMCGR, RegValueBACKCR, RegValueSTCR, RegValueDACSPICR, temp = 0;

    ui8ModFrqOffset &= 0x7F;

    am_devices_cooper_plf_reg_read(pBLE, 0x43000004, &RegValueMCGR);

    //
    // Unlock the BLE registers.
    //
    am_devices_cooper_plf_reg_write(pBLE, 0x43000004, 0xFFFFFFFF);
    am_devices_cooper_plf_reg_read(pBLE, 0x52000008, &temp);
    temp |= 0x08;
    am_devices_cooper_plf_reg_read(pBLE, 0x52000000, &RegValueSTCR);
    RegValueSTCR |= (1 << 10);
    am_devices_cooper_plf_reg_write(pBLE, 0x52000000, RegValueSTCR);

    am_devices_cooper_plf_reg_read(pBLE, 0x45800070, &RegValueBACKCR);
    am_devices_cooper_plf_reg_write(pBLE, 0x45800070, (RegValueBACKCR | 0x8));
    RegValueDACSPICR = (ui8ModFrqOffset << 1) | 0x1;
    am_devices_cooper_plf_reg_write(pBLE, 0x52000014, RegValueDACSPICR);

    am_devices_cooper_plf_reg_write(pBLE, 0x52000008, temp);

    if (APOLLO3_B0)
    {
        am_devices_cooper_plf_reg_write(pBLE, AM_HAL_BLE_IP_RAM_MODEX_TRIM_ADDR_B0, ui8ModFrqOffset);
    }
    else
    {
        am_devices_cooper_plf_reg_write(pBLE, AM_HAL_BLE_IP_RAM_MODEX_TRIM_ADDR_A1, ui8ModFrqOffset);
    }
    am_devices_cooper_plf_reg_write(pBLE, 0x43000004, RegValueMCGR);
#endif
    return AM_DEVICES_COOPER_STATUS_SUCCESS;
} // am_util_ble_transmitter_modex_set()

//*****************************************************************************
//
// Load the modulation frequency offset from INFO1,
// based on the tested values stored in non-volatile memory.
//
//*****************************************************************************
uint8_t
am_util_ble_read_trimdata_from_info1(void)
{
    uint8_t TrimData = 0;
    //TODO
#if 0
    uint32_t ui32TrimValue = 0, temp = 0;

    temp = ui32TrimValue = AM_REGVAL(0x50023808);
    temp &= 0xffffff00;

    if ( temp == 0x18240600 )
    {
        TrimData = ui32TrimValue & 0xFF;
    }
    else
    {
        TrimData = 0;
    }

    if ( (TrimData > 0x50) || (TrimData < 0x20) )   // change from 0x40 to 0x50 for improving the FT2 yield.
    {
        TrimData = 0;
    }
#endif
    return TrimData;
} // am_util_ble_read_trimdata_from_info1()

//*****************************************************************************
//
// Set the modulation frequency offset from INFO1,
// based on the tested values stored in non-volatile memory.
//
//*****************************************************************************
uint32_t
am_util_ble_load_modex_trim_set(void *pHandle)
{
    uint8_t ui8TrimValue;
    //
    // load the modex trim data from info1.
    //
    ui8TrimValue = am_util_ble_read_trimdata_from_info1();
    if ( ui8TrimValue )
    {
        am_util_ble_transmitter_modex_set(pHandle, ui8TrimValue);
        return AM_DEVICES_COOPER_STATUS_SUCCESS;
    }
    else
    {
        return AM_DEVICES_COOPER_STATUS_ERROR;
    }
} // am_util_ble_load_modex_trim_set()

//*****************************************************************************
//
//to do directly output modulation signal. change channel ranges from 0 to 0x27, pattern from 0 to 7.
//
//*****************************************************************************
uint32_t
am_util_ble_set_adv_mode(void *pHandle)
{
#if ((defined(AM_PART_APOLLO4)) && (defined(USE_NONBLOCKING_HCI)))
    AM_SHARED_RW am_devices_cooper_buffer(20) sWriteCommand;
    AM_SHARED_RW am_devices_cooper_buffer(20) sResponse;
#else
    am_devices_cooper_buffer(20) sWriteCommand;
    am_devices_cooper_buffer(20) sResponse;
#endif
    uint32_t ui32Status;
    uint8_t pui8Parameter[15] = {0x20, 0, 0x20, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0};

    am_devices_cooper_sleep_set(pHandle, false);
    am_util_delay_ms(1);
    am_devices_cooper_tx_power_set(pHandle,0);
    am_util_delay_ms(1);

    // issue the HCI command with to TX carrier wave
    //01 06 20 0F 20 00 20 00 00 00 00 00 00 00 00 00 00 07 00
    ui32Status = am_devices_cooper_command_write(pHandle, sWriteCommand.words, sResponse.words, 0x2006, 19, pui8Parameter);
    if ( ui32Status != AM_DEVICES_COOPER_STATUS_SUCCESS )
    {
        return ui32Status;
    }

    // issue the HCI command with to TX carrier wave
    //01 0A 20 01 01
    memset(pui8Parameter, 0, 15);
    pui8Parameter[0] = 1;
    ui32Status = am_devices_cooper_command_write(pHandle, sWriteCommand.words, sResponse.words, 0x200A, 5, pui8Parameter);
    if ( ui32Status != AM_DEVICES_COOPER_STATUS_SUCCESS )
    {
        return ui32Status;
    }
    return AM_DEVICES_COOPER_STATUS_SUCCESS;
}

