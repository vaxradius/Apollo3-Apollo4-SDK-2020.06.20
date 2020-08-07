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

#include "main.h"


extern int nemadc_4layer_test(void);

//*****************************************************************************
//
// Main Function
//
//*****************************************************************************

int main(void)
{
#ifdef ENABLE_DSI
    //
    // Enable DSI power and configure DSI clock.
    //
    am_hal_dsi_config();
#else
    am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_DISPCLKSEL_HFRC12, NULL);
    am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_DCCLK_ENABLE, NULL);
#endif
    am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_PERIPH_GFX);
    am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_PERIPH_DISP);

    CPU->DAXICFG_b.BUFFERENABLE = CPU_DAXICFG_BUFFERENABLE_ONE;  // only enable a single buffer

    PWRCTRL->SSRAMPWREN = PWRCTRL_SSRAMPWREN_PWRENSSRAM_ALL;  // enable all
    while(PWRCTRL->SSRAMPWRST == 0);

#if 0
    TIMER->TMR0CMP0 = 0x1000000;
    TIMER->CTRL0 = 0x11;  // enable timer
#else
    am_hal_timer_config_t       TimerConfig;
    am_hal_timer_default_config_set(&TimerConfig);
//    TimerConfig.eInputClock = AM_HAL_TIMER_CLOCK_HFRC_DIV4K;
    TimerConfig.eInputClock = AM_HAL_TIMER_CLOCK_HFRC_DIV16;
    TimerConfig.eFunction = AM_HAL_TIMER_FN_EDGE;

    am_hal_timer_config(0, &TimerConfig);
    am_hal_timer_start(0);
#endif



#ifdef BAREMETAL
    nemadc_4layer_test();
#else /* BAREMETAL */
    // Run the application.
    run_tasks();
#endif /* BAREMETAL*/

    while (1)
    {
    }
}

