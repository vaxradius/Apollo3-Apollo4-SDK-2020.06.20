//*****************************************************************************
//
//! @file am_hal_dsi.c
//!
//! @brief Hardware abstraction for the DSI
//!
//! @addtogroup
//! @ingroup
//! @{
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
#include "am_mcu_apollo.h"

//*****************************************************************************
//
// Initialize the DSI
//
//*****************************************************************************
uint32_t
am_hal_dsi_initialize(uint8_t ui8LanesNum, uint8_t ui8DBIBusWidth)
{

    //
    // ui32DSIFuncPrg_REG (DATA_WIDTH, RESERVED, FMT_VIDEO, CH_NO_CM, CH_NO_VM, DATA_LANE_CNT)
    //                   [15:13]     [12:10]   [9:7]      [6:5]     [4:3]     [2:0] 
    //
    uint32_t ui32DSIFuncPrg = 0; 
    //
    // check number of lanes parameters
    //
    switch (ui8LanesNum)
    {
        case 1:
          ui32DSIFuncPrg |= 0x1;
          break;

        case 2:
          ui32DSIFuncPrg |= 0x2;
          break;

        default:
          return AM_HAL_STATUS_OUT_OF_RANGE;
    }
    //
    // check DBI bus width parameter
    //
    switch (ui8DBIBusWidth)
    {
        case 8:
          ui32DSIFuncPrg |= 0x6000;
          break;

        case 9:
          ui32DSIFuncPrg |= 0x4000;
          break;

        case 16:
          ui32DSIFuncPrg |= 0x2000;
          break;

        default:
          return AM_HAL_STATUS_OUT_OF_RANGE;
    }

    DSI->RSTENBDFE = 0x0;
    //
    // write into DSI functional programming register
    //
    DSI->DSIFUNCPRG = ui32DSIFuncPrg;
    //
    // write into HIGH SPEED RECEIVE TIMEOUT REGISTER
    //
    DSI->HSTXTIMEOUT = 0x00FFFFFF;
    //
    //write into LOW POWER RECEIVE TIMEOUT REGISTER
    //
    DSI->LPRXTO = 0xFF;
    //
    // write into TURN AROUND TIMEOUT REGISTER
    //
    DSI->TURNARNDTO = 0x1F;
    //
    // write into DEVICE RESET TIMER REGISTER
    //
    DSI->DEVICERESETTIMER = 0xFF;
    //
    // write into HIGH TO LOW SWITCH COUNT REGISTER
    //
    DSI->DATALANEHILOSWCNT = 0xFFFF;
    DSI->INITCNT = 0x7d0;
    DSI->LPBYTECLK = 0x3;
    //
    // DPHY DATA parameters
    //
    DSI->DPHYPARAM = 0x0C050A05;
    //
    // DPHY CLK parameters
    //
    DSI->CLKLANETIMPARM = 0x04030C02;
    // 
    // PLL settings
    //
    int32_t trim1 = (DSI->AFETRIM1) & 0xFFFFFF80;
    DSI->AFETRIM1 = trim1 | 0x0A; // 60MHz x 4 = 240MHz
    DSI->AFETRIM2 = 0x10000000;
    //
    // enable DPHY
    //
    DSI->RSTENBDFE = 0x1;
    DSI->DEVICEREADY = 0x1;
    //
    // Wait for DPHY init
    //
    am_hal_delay_us(200);
    //
    // Return the status.
    //
    return AM_HAL_STATUS_SUCCESS;
}

//*****************************************************************************
//
// Cofigure power and clock of DSI
//
//*****************************************************************************
uint32_t
am_hal_dsi_config(void)
{
    am_hal_delay_us(5000000); 
    uint32_t ui32Val = AM_REGVAL(0x40020088);
    AM_REGVAL(0x40020088) = ui32Val - 21; // FIXME - Adjust VDDF to ~720mV, in order to meet timming requirments.

    am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_PERIPH_DISPPHY);

    am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_DISPCLKSEL_DPHYPLL, NULL);
    am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_DCCLK_ENABLE, NULL);
    am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_PLLCLKSEL_HFRC12, NULL); 
    am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_PLLCLK_ENABLE, NULL);
    return AM_HAL_STATUS_SUCCESS;
}
//*****************************************************************************
//
// End Doxygen group.
//
//*****************************************************************************
