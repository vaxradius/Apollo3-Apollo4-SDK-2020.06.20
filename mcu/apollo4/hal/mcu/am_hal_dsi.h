//*****************************************************************************
//
//! @file am_hal_dsi.h
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

#ifndef AM_HAL_DSI_H
#define AM_HAL_DSI_H

#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// DSI error codes.
//
//*****************************************************************************
/*typedef enum
{
    AM_HAL_DSI_INT_STATUS_RX_START_TRANS_ERROR = AM_HAL_STATUS_MODULE_SPECIFIC_START,
    AM_HAL_DSI_INT_STATUS_RX_START_TRANS_SYNC_ERROR,
    AM_HAL_DSI_INT_STATUS_RX_END_TRANS_SYNC_ERROR,
    AM_HAL_DSI_INT_STATUS_RX_ESCAPE_ENTRY_ERROR,
    AM_HAL_DSI_INT_STATUS_RX_LP_TX_SYNC_ERROR,
    AM_HAL_DSI_INT_STATUS_RX_PERIPH_TIMEOUT_ERROR,
    AM_HAL_DSI_INT_STATUS_RX_FALSE_CONTROL_ERROR,
    AM_HAL_DSI_INT_STATUS_RX_ECC_SINGLE_BIT_ERROR,
    AM_HAL_DSI_INT_STATUS_RX_ECC_MULTI_BIT_ERROR,
    AM_HAL_DSI_INT_STATUS_START_TRANS_SYNC_ERROR,

}
am_hal_dsi_interrupt_status_t;*/

//! @}

//*****************************************************************************
//
// External functions.
//
//*****************************************************************************
//*****************************************************************************
//
//! @brief DSI initialization
//!
//! @param ui8LanesNum is number of lanes.
//!
//! @param ui8DBIBusWidth is width of DBI bus.
//!
//! This function should be called after DSI power is enabled.
//!
//! @return AM_HAL_STATUS_SUCCESS
//
//*****************************************************************************
extern uint32_t
am_hal_dsi_initialize(uint8_t ui8LanesNum, uint8_t ui8DBIBusWidth);

//*****************************************************************************
//
//! @brief DSI configuration
//!
//! Configure power and clock of DSI.
//!
//! @return AM_HAL_STATUS_SUCCESS
//
//*****************************************************************************
extern uint32_t
am_hal_dsi_config(void);

#ifdef __cplusplus
}
#endif

#endif // AM_HAL_DSI_H

//*****************************************************************************
//
// End Doxygen group.
//! @}
//
//*****************************************************************************
