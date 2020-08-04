//*****************************************************************************
//
//  am_hal_mcuctrl.c
//! @file
//!
//! @brief Functions for interfacing with the MCUCTRL.
//!
//! @addtogroup mcuctrl3 MCU Control (MCUCTRL)
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
#include "am_util.h"

//*****************************************************************************
//
// Global Variables.
//
//*****************************************************************************
//
// Define the MRAM sizes from CHIPPN.
//
const uint32_t
g_am_hal_mcuctrl_nv_size[AM_HAL_MCUCTRL_CHIPPN_NV_SIZE_N] =
{
     16 * 1024,             /* 0x0 0x00004000   16 KB */
     32 * 1024,             /* 0x1 0x00008000   32 KB */
     64 * 1024,             /* 0x2 0x00010000   64 KB */
    128 * 1024,             /* 0x3 0x00020000  128 KB */
    256 * 1024,             /* 0x4 0x00040000  256 KB */
    512 * 1024,             /* 0x5 0x00080000  512 KB */
      1 * 1024 * 1024,      /* 0x6 0x00100000    1 MB */
      2 * 1024 * 1024,      /* 0x7 0x00200000    2 MB */
};

const uint32_t
g_am_hal_mcuctrl_dtcm_size[AM_HAL_MCUCTRL_CHIPPN_DTCM_SIZE_N] =
{
     16 * 1024,             /* 0x0 0x00004000   16 KB */
     32 * 1024,             /* 0x1 0x00008000   32 KB */
     64 * 1024,             /* 0x2 0x00010000   64 KB */
    128 * 1024,             /* 0x3 0x00020000  128 KB */
    256 * 1024,             /* 0x4 0x00040000  256 KB */
    512 * 1024,             /* 0x5 0x00080000  512 KB */
      1 * 1024 * 1024,      /* 0x6 0x00100000    1 MB */
    384 * 1024,             /* 0x7 0x00200000  384 KB */
};

const uint32_t
g_am_hal_mcuctrl_ssram_size[AM_HAL_MCUCTRL_SSRAM_MAX + 1] =
{
     512 * 1024,
     1024 * 1024,
     (1024 + 480) * 1024, // Extended memory is 480K
//   (1024 + 480) * 1024 // Extended memory is 480K
};

const uint32_t
g_am_hal_mcuctrl_mram_size[AM_HAL_MCUCTRL_MRAM_MAX + 1] =
{
     512  * 1024,
     1024 * 1024,
     1536 * 1024,
     2048 * 1024
};


// ****************************************************************************
//
//  device_info_get()
//  Gets all relevant device information.
//
// ****************************************************************************
static void
device_info_get(am_hal_mcuctrl_device_t *psDevice)
{
    //
    // Read the Part Number.
    //
    psDevice->ui32ChipPN = MCUCTRL->CHIPPN;

    //
    // Read the Chip ID0.
    //
    psDevice->ui32ChipID0 = MCUCTRL->CHIPID0;

    //
    // Read the Chip ID1.
    //
    psDevice->ui32ChipID1 = MCUCTRL->CHIPID1;

    //
    // Read the Chip Revision.
    //
    psDevice->ui32ChipRev = MCUCTRL->CHIPREV;

    //
    // Read the Chip VENDOR ID.
    //
    psDevice->ui32VendorID = MCUCTRL->VENDORID;

    //
    // Read the SKU (new for Apollo3).
    //
    psDevice->ui32SKU = MCUCTRL->SKU;

    //
    // Qualified from Part Number.
    //
    psDevice->ui32Qualified = (psDevice->ui32ChipPN >> MCUCTRL_CHIPPN_PARTNUM_QUAL_S) & 0x1;

    //
    // DTCM size from Part Number.
    //
    psDevice->ui32DTCMSize = 384 * 1024;

    //
    // Shared SRAM
    //
    if ( MCUCTRL->SKU_b.SKUSRAMSIZE <= AM_HAL_MCUCTRL_SSRAM_MAX )
    {
        psDevice->ui32SSRAMSize = g_am_hal_mcuctrl_ssram_size[MCUCTRL->SKU_b.SKUSRAMSIZE];
    }
    else
    {
        psDevice->ui32SSRAMSize = g_am_hal_mcuctrl_ssram_size[AM_HAL_MCUCTRL_SSRAM_MAX];
        MCUCTRL->SKU_b.SKUSRAMSIZE = AM_HAL_MCUCTRL_SSRAM_MAX;
    }

    //
    // MRAM
    //
    if ( MCUCTRL->SKU_b.SKUMRAMSIZE <= AM_HAL_MCUCTRL_MRAM_MAX )
    {
        psDevice->ui32MRAMSize = g_am_hal_mcuctrl_mram_size[MCUCTRL->SKU_b.SKUMRAMSIZE];
    }
    else
    {
        psDevice->ui32MRAMSize = g_am_hal_mcuctrl_mram_size[AM_HAL_MCUCTRL_MRAM_MAX];
        MCUCTRL->SKU_b.SKUMRAMSIZE = AM_HAL_MCUCTRL_MRAM_MAX;
    }

    // Now, let's look at the JEDEC info.
    // The full partnumber is 12 bits total, but is scattered across 2 registers.
    // Bits [11:8] are 0xE.
    // Bits [7:4] are 0xE for Apollo, 0xD for Apollo2.
    // Bits [3:0] are defined differently for Apollo and Apollo2.
    //   For Apollo, the low nibble is 0x0.
    //   For Apollo2, the low nibble indicates flash and SRAM size.
    //
    psDevice->ui32JedecPN  = JEDEC->PID0_b.PNL8 << 0;
    psDevice->ui32JedecPN |= JEDEC->PID1_b.PNH4 << 8;

    //
    // JEPID is the JEP-106 Manufacturer ID Code, which is assigned to Ambiq as
    //  0x1B, with parity bit is 0x9B.  It is 8 bits located across 2 registers.
    //
    psDevice->ui32JedecJEPID  = JEDEC->PID1_b.JEPIDL << 0;
    psDevice->ui32JedecJEPID |= JEDEC->PID2_b.JEPIDH << 4;

    //
    // CHIPREV is 8 bits located across 2 registers.
    //
    psDevice->ui32JedecCHIPREV  = JEDEC->PID2_b.CHIPREVH4 << 4;
    psDevice->ui32JedecCHIPREV |= JEDEC->PID3_b.CHIPREVL4 << 0;

    //
    // Let's get the Coresight ID (32-bits across 4 registers)
    // For Apollo and Apollo2, it's expected to be 0xB105100D.
    //
    psDevice->ui32JedecCID  = JEDEC->CID3_b.CID << 24;
    psDevice->ui32JedecCID |= JEDEC->CID2_b.CID << 16;
    psDevice->ui32JedecCID |= JEDEC->CID1_b.CID <<  8;
    psDevice->ui32JedecCID |= JEDEC->CID0_b.CID <<  0;

} // device_info_get()

// ****************************************************************************
//
//  am_hal_mcuctrl_control()
//  Apply various specific commands/controls on the MCUCTRL module.
//
// ****************************************************************************
uint32_t
am_hal_mcuctrl_control(am_hal_mcuctrl_control_e eControl, void *pArgs)
{
    volatile uint32_t ui32Reg;

    switch ( eControl )
    {
        case AM_HAL_MCUCTRL_CONTROL_EXTCLK32K_ENABLE:
            //
            // Configure the bits in XTALCTRL that enable external 32KHz clock.
            //
            MCUCTRL->XTALCTRL_b.XTALPDNB = MCUCTRL_XTALCTRL_XTALPDNB_PWRUPCORE;
            MCUCTRL->XTALCTRL_b.XTALCOMPPDNB = MCUCTRL_XTALCTRL_XTALCOMPPDNB_PWRUPCOMP;
            MCUCTRL->XTALCTRL_b.XTALCOMPBYPASS = MCUCTRL_XTALCTRL_XTALCOMPBYPASS_BYPCOMP;
            MCUCTRL->XTALCTRL_b.XTALCOREDISFB = MCUCTRL_XTALCTRL_XTALCOREDISFB_DIS;
            MCUCTRL->XTALCTRL_b.XTALSWE = MCUCTRL_XTALCTRL_XTALSWE_OVERRIDE_EN;
            break;

        case AM_HAL_MCUCTRL_CONTROL_EXTCLK32K_DISABLE:
            //
            // Configure the bits in XTALCTRL that disable external 32KHz
            // clock, thus re-configuring for the crystal.
            //
            MCUCTRL->XTALCTRL_b.XTALPDNB = MCUCTRL_XTALCTRL_XTALPDNB_PWRUPCORE;
            MCUCTRL->XTALCTRL_b.XTALCOMPPDNB = MCUCTRL_XTALCTRL_XTALCOMPPDNB_PWRUPCOMP;
            MCUCTRL->XTALCTRL_b.XTALCOMPBYPASS = MCUCTRL_XTALCTRL_XTALCOMPBYPASS_USECOMP;
            MCUCTRL->XTALCTRL_b.XTALCOREDISFB = MCUCTRL_XTALCTRL_XTALCOREDISFB_EN;
            MCUCTRL->XTALCTRL_b.XTALSWE = MCUCTRL_XTALCTRL_XTALSWE_OVERRIDE_DIS;
            break;

        case AM_HAL_MCUCTRL_CONTROL_EXTCLK32M_KICK_START:
//            MCUCTRL->XTALHSTRIMS = 0x0FFF8D2C;
            MCUCTRL->XTALHSTRIMS_b.XTALHSCAP2TRIM = 44;
            MCUCTRL->XTALHSTRIMS_b.XTALHSCAPTRIM  = 4;
            MCUCTRL->XTALHSTRIMS_b.XTALHSDRIVETRIM = 3;
            MCUCTRL->XTALHSTRIMS_b.XTALHSDRIVERSTRENGTH = 0;
            MCUCTRL->XTALHSTRIMS_b.XTALHSIBIASCOMP2TRIM = 3;
            MCUCTRL->XTALHSTRIMS_b.XTALHSIBIASCOMPTRIM = 15;
            MCUCTRL->XTALHSTRIMS_b.XTALHSIBIASTRIM = 127;
            MCUCTRL->XTALHSTRIMS_b.XTALHSRSTRIM = 0;
            MCUCTRL->XTALHSTRIMS_b.XTALHSSPARE = 0;

//            MCUCTRL->XTALHSCTRL  = 0x0A2;
            MCUCTRL->XTALHSCTRL_b.XTALHSCOMPPDNB = 1;
            MCUCTRL->XTALHSCTRL_b.XTALHSPDNPNIMPROVE = 1;
            MCUCTRL->XTALHSCTRL_b.XTALHSPADOUTEN = 1;

            //
            // Turn on xtalhs_pdnb & xtalhs_injection_enable
            //
            MCUCTRL->XTALHSCTRL_b.XTALHSPDNB = 1;
            MCUCTRL->XTALHSCTRL_b.XTALHSINJECTIONENABLE = 1;

            //
            // Turn on xtalhs_ibst_enable
            //
            MCUCTRL->XTALHSCTRL_b.XTALHSIBSTENABLE = 1;

            //
            // Turn off xtalhs_injection_enable
            //
            am_util_delay_us(5);
            MCUCTRL->XTALHSCTRL_b.XTALHSINJECTIONENABLE = 0;

            //
            // Turn off xtalhs_ibst_enable and turn off xtalhs_rs_trim
            //
            am_util_delay_us(500);

//            MCUCTRL->XTALHSTRIMS = 0x1311811C;
            MCUCTRL->XTALHSTRIMS_b.XTALHSCAP2TRIM = 28;
            MCUCTRL->XTALHSTRIMS_b.XTALHSCAPTRIM  = 4;
            MCUCTRL->XTALHSTRIMS_b.XTALHSDRIVETRIM = 0;
            MCUCTRL->XTALHSTRIMS_b.XTALHSDRIVERSTRENGTH = 0;
            MCUCTRL->XTALHSTRIMS_b.XTALHSIBIASCOMP2TRIM = 3;
            MCUCTRL->XTALHSTRIMS_b.XTALHSIBIASCOMPTRIM = 8;
            MCUCTRL->XTALHSTRIMS_b.XTALHSIBIASTRIM = 24;
            MCUCTRL->XTALHSTRIMS_b.XTALHSRSTRIM = 1;
            MCUCTRL->XTALHSTRIMS_b.XTALHSSPARE = 0;

            MCUCTRL->XTALHSCTRL_b.XTALHSIBSTENABLE = 0;
            // Apply external source
            if ( (pArgs) && (*((bool *)pArgs) == true) )
            {
                MCUCTRL->XTALHSCTRL_b.XTALHSPDNB = 0;
                MCUCTRL->XTALHSCTRL_b.XTALHSEXTERNALCLOCK = 1;
            }
            break;

        case AM_HAL_MCUCTRL_CONTROL_EXTCLK32M_NORMAL:
//            MCUCTRL->XTALHSTRIMS = 0x1311811C;
            MCUCTRL->XTALHSTRIMS_b.XTALHSCAP2TRIM = 28;
            MCUCTRL->XTALHSTRIMS_b.XTALHSCAPTRIM  = 4;
            MCUCTRL->XTALHSTRIMS_b.XTALHSDRIVETRIM = 0;
            MCUCTRL->XTALHSTRIMS_b.XTALHSDRIVERSTRENGTH = 0;
            MCUCTRL->XTALHSTRIMS_b.XTALHSIBIASCOMP2TRIM = 3;
            MCUCTRL->XTALHSTRIMS_b.XTALHSIBIASCOMPTRIM = 8;
            MCUCTRL->XTALHSTRIMS_b.XTALHSIBIASTRIM = 24;
            MCUCTRL->XTALHSTRIMS_b.XTALHSRSTRIM = 1;
            MCUCTRL->XTALHSTRIMS_b.XTALHSSPARE = 0;

//            MCUCTRL->XTALHSCTRL  = 0x0A2;
            MCUCTRL->XTALHSCTRL_b.XTALHSCOMPPDNB = 1;
            MCUCTRL->XTALHSCTRL_b.XTALHSPDNPNIMPROVE = 1;
            MCUCTRL->XTALHSCTRL_b.XTALHSPADOUTEN = 1;

            //
            // Turn on xtalhs_pdnb
            //
            MCUCTRL->XTALHSCTRL_b.XTALHSPDNB = 1;
            // Apply external source
            if ( (pArgs) && (*((bool *)pArgs) == true) )
            {
                MCUCTRL->XTALHSCTRL_b.XTALHSPDNB = 0;
                MCUCTRL->XTALHSCTRL_b.XTALHSEXTERNALCLOCK = 1;
            }
            break;

        case AM_HAL_MCUCTRL_CONTROL_EXTCLK32M_DISABLE:
//            MCUCTRL->XTALHSTRIMS = 0x0311F12C;
            MCUCTRL->XTALHSTRIMS = ui32Reg;
            MCUCTRL->XTALHSTRIMS_b.XTALHSCAP2TRIM = 44;
            MCUCTRL->XTALHSTRIMS_b.XTALHSCAPTRIM  = 4;
            MCUCTRL->XTALHSTRIMS_b.XTALHSDRIVETRIM = 0;
            MCUCTRL->XTALHSTRIMS_b.XTALHSDRIVERSTRENGTH = 7;
            MCUCTRL->XTALHSTRIMS_b.XTALHSIBIASCOMP2TRIM = 3;
            MCUCTRL->XTALHSTRIMS_b.XTALHSIBIASCOMPTRIM = 8;
            MCUCTRL->XTALHSTRIMS_b.XTALHSIBIASTRIM = 24;
            MCUCTRL->XTALHSTRIMS_b.XTALHSRSTRIM = 0;
            MCUCTRL->XTALHSTRIMS_b.XTALHSSPARE = 0;

//            MCUCTRL->XTALHSCTRL  = 0x0A2;
            MCUCTRL->XTALHSCTRL_b.XTALHSPDNB = 0;
            MCUCTRL->XTALHSCTRL_b.XTALHSEXTERNALCLOCK = 0;
            MCUCTRL->XTALHSCTRL_b.XTALHSCOMPPDNB = 1;
            MCUCTRL->XTALHSCTRL_b.XTALHSPDNPNIMPROVE = 1;
            MCUCTRL->XTALHSCTRL_b.XTALHSPADOUTEN = 1;

            break;

        default:
            return AM_HAL_STATUS_INVALID_ARG;
    }

    //
    // Return success status.
    //
    return AM_HAL_STATUS_SUCCESS;

} // am_hal_mcuctrl_control()

// ****************************************************************************
//
//  am_hal_mcuctrl_status_get()
//! This function returns  current status of the MCUCTRL as obtained from
//! various registers of the MCUCTRL block.
//
// ****************************************************************************
uint32_t
am_hal_mcuctrl_status_get(am_hal_mcuctrl_status_t *psStatus)
{
    uint32_t ui32Status;

    if ( psStatus == NULL )
    {
        return AM_HAL_STATUS_INVALID_ARG;
    }

    psStatus->bDebuggerLockout =
        _FLD2VAL(MCUCTRL_DEBUGGER_LOCKOUT, MCUCTRL->DEBUGGER);

    psStatus->bADCcalibrated =
        _FLD2VAL(MCUCTRL_ADCCAL_ADCCALIBRATED, MCUCTRL->ADCCAL);

    psStatus->bBattLoadEnabled =
        _FLD2VAL(MCUCTRL_ADCBATTLOAD_BATTLOAD, MCUCTRL->ADCBATTLOAD);

    ui32Status = MCUCTRL->BOOTLOADER;
    psStatus->bSecBootOnColdRst =
        _FLD2VAL(MCUCTRL_BOOTLOADER_SECBOOT, ui32Status);
    psStatus->bSecBootOnWarmRst =
        _FLD2VAL(MCUCTRL_BOOTLOADER_SECBOOTONRST, ui32Status);

    return AM_HAL_STATUS_SUCCESS;

} // am_hal_mcuctrl_status_get()

// ****************************************************************************
//
//  am_hal_mcuctrl_info_get()
//  Get information of the given MCUCTRL item.
//
// ****************************************************************************
uint32_t
am_hal_mcuctrl_info_get(am_hal_mcuctrl_infoget_e eInfoGet, void *pInfo)
{
    am_hal_mcuctrl_feature_t *psFeature;

    if ( pInfo == NULL )
    {
        return AM_HAL_STATUS_INVALID_ARG;
    }

    switch ( eInfoGet )
    {
        case AM_HAL_MCUCTRL_INFO_FEATURES_AVAIL:
            psFeature = (am_hal_mcuctrl_feature_t*)pInfo;
            psFeature->eDTCMSize = AM_HAL_MCUCTRL_DTCM_384K;
            psFeature->eSharedSRAMSize = (am_hal_mcuctrl_ssram_e)MCUCTRL->SKU_b.SKUMRAMSIZE;
            psFeature->eMRAMSize = (am_hal_mcuctrl_mram_e)MCUCTRL->SKU_b.SKUSRAMSIZE;
            psFeature->bTurboSpot = (MCUCTRL->SKU_b.SKUTURBOSPOT > 0);
            psFeature->bDisplayCtrl = (MCUCTRL->SKU_b.SKUMIPIDSI > 0);
            psFeature->bGPU = (MCUCTRL->SKU_b.SKUGFX > 0);
            psFeature->bUSB = (MCUCTRL->SKU_b.SKUUSB > 0);
            psFeature->bSecBootFeature = (MCUCTRL->SKU_b.SKUSECURESPOT > 0);
            break;

        case AM_HAL_MCUCTRL_INFO_DEVICEID:
            device_info_get((am_hal_mcuctrl_device_t *)pInfo);
            break;

        default:
            return AM_HAL_STATUS_INVALID_ARG;
    }

    //
    // Return success status.
    //
    return AM_HAL_STATUS_SUCCESS;

} // am_hal_mcuctrl_info_get()

//*****************************************************************************
//
// End Doxygen group.
//! @}
//
//*****************************************************************************
