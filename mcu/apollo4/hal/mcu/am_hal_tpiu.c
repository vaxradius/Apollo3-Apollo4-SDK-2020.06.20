//*****************************************************************************
//
//  am_hal_tpiu.c
//! @file
//!
//! @brief Support functions for the ARM TPIU module
//!
//! Provides support functions for configuring the ARM TPIU module
//!
//! @addtogroup tpiu3 Trace Port Interface Unit (TPIU)
//! @ingroup apollo3hal
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
//! @brief Enables the TPIU
//!
//! This function enables the ARM TPIU by setting the TPIU registers and then
//! enabling the TPIU clock source in MCU control register.
//!
//! @param psConfig - structure for configuration.
//!     If ui32SetItmBaud, the other structure members are used to set the
//!      TPIU configuration.
//!     But for simplicity, ui32SetItmBaud can be set to one of the
//!      following, in which case all other structure members are ignored.
//!      In this case, the given BAUD rate is based on a div-by-8 HFRC clock.
//!         AM_HAL_TPIU_BAUD_57600
//!         AM_HAL_TPIU_BAUD_115200
//!         AM_HAL_TPIU_BAUD_230400
//!         AM_HAL_TPIU_BAUD_460800
//!         AM_HAL_TPIU_BAUD_500000
//!         AM_HAL_TPIU_BAUD_1M
//!
//! @return None.
//
//*****************************************************************************
void
am_hal_tpiu_enable(uint32_t ui32SetItmBaud)
{
#define CLKGEN_DEFINED 0
#if CLKGEN_DEFINED  // TODO FIXME - Once CLKGEN is defined.
    uint32_t ui32HFRC, ui32SWOscaler;
#endif
    uint32_t ui32SWOscaler, ui32ITMbitrate;

    ui32ITMbitrate = ui32SetItmBaud;

    //
    // TPIU formatter & flush control register.
    //
    TPI->FFCR = 0;

    //
    // Set the Current Parallel Port Size (note - only 1 bit can be set).
    //
    TPI->CSPSR = TPI_CSPSR_CWIDTH_1BIT;

    //
    // Use some default assumptions to set the ITM frequency.
    //
    if ( (ui32ITMbitrate < AM_HAL_TPIU_BAUD_57600 )  ||
         (ui32ITMbitrate > AM_HAL_TPIU_BAUD_2M ) )
    {
        ui32ITMbitrate = AM_HAL_TPIU_BAUD_DEFAULT;
    }

#if APOLLO4_FPGA
    //
    // Compute an appropriate SWO scaler based on the FPGA speed
    // specified by the APOLLO4_FPGA (e.g. 48, 24, 12, ...)
    //
    if ( g_ui32FPGAfreqMHz == 0 )
    {
        g_ui32FPGAfreqMHz = APOLLO4_FPGA;
    }

    ui32SWOscaler = (g_ui32FPGAfreqMHz / 2) - 1;
#else
    ui32SWOscaler = ( (AM_HAL_CLKGEN_FREQ_MAX_HZ / 4) /
                       AM_HAL_TPIU_BAUD_DEFAULT ) - 1;
#endif // APOLLO4_FPGA

  //
  // Set the scaler value.
  //
#if 1 // FIXME TODO: These should be defined in am_reg_m4.h
#define TPI_CSPSR_CWIDTH_1BIT      1
#define TPI_SPPR_TXMODE_UART       2
#define TPI_ITCTRL_Mode_NORMAL     0
#ifndef TPI_ACPR_SWOSCALER_Pos
#define TPI_ACPR_SWOSCALER_Pos              0U                                         /*!< TPI ACPR: SWOSCALER Position */
#define TPI_ACPR_SWOSCALER_Msk             (0xFFFFUL /*<< TPI_ACPR_SWOSCALER_Pos*/)    /*!< TPI ACPR: SWOSCALER Mask */
#endif
#endif
    TPI->ACPR = _VAL2FLD(TPI_ACPR_SWOSCALER, ui32SWOscaler);

    //
    // Set the Pin Protocol.
    //
    TPI->SPPR = _VAL2FLD( TPI_SPPR_TXMODE, TPI_SPPR_TXMODE_UART);  // NRZ

    //
    // Make sure we are not in test mode (important for proper deep sleep
    // operation).
    //
    TPI->ITCTRL = _VAL2FLD(TPI_ITCTRL_Mode, TPI_ITCTRL_Mode_NORMAL);

    //
    // Enable the TPIU clock source in MCU control.
    // Set TPIU clock for HFRC/16 (96MHz/16 = 6MHz) operation.
    //
    MCUCTRL->DBGCTRL_b.CM4CLKSEL     = MCUCTRL_DBGCTRL_CM4CLKSEL_HFRCDIV2;
    MCUCTRL->DBGCTRL_b.CM4TPIUENABLE = MCUCTRL_DBGCTRL_CM4TPIUENABLE_EN;

    //
    // Wait for 50us for the data to flush out.
    //
    am_hal_delay_us(50);
} // am_hal_tpiu_enable()

//*****************************************************************************
//
//! @brief Disables the TPIU
//!
//! This function disables the ARM TPIU by disabling the TPIU clock source
//! in MCU control register.
//!
//! @return None.
//
//*****************************************************************************
void
am_hal_tpiu_disable(void)
{
    //
    // Disable the CM4 TPIU clock source in MCU control.
    //
    MCUCTRL->DBGCTRL_b.CM4CLKSEL = MCUCTRL_DBGCTRL_CM4CLKSEL_LOWPWR;
    MCUCTRL->DBGCTRL_b.CM4TPIUENABLE = MCUCTRL_DBGCTRL_CM4TPIUENABLE_DIS;
} // am_hal_tpiu_disable()

//*****************************************************************************
//
// End Doxygen group.
//! @}
//
//*****************************************************************************
