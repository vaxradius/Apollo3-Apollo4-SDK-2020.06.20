//*****************************************************************************
//
//! @file binary_counter.c
//!
//! @brief Example that displays the timer count on the LEDs.
//!
//! Purpose: This example increments a variable on every timer interrupt. The global
//! variable is used to set the state of the LEDs. The example sleeps otherwise.
//!
//! Printing takes place over the ITM at 1M Baud.
//!
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

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

//*****************************************************************************
//
// Macros
//
//*****************************************************************************
//
// The default is to use the LFRC as the clock source.
// Can  use the XTAL via the USE_XTAL define.
//
//#define USE_XTAL    1
#if USE_XTAL
#define BC_CLKSRC   "XTAL"
#else
#define BC_CLKSRC   "LFRC"
#endif

#ifdef AM_BSP_NUM_LEDS
#define NUM_LEDS    AM_BSP_NUM_LEDS
#else
#define NUM_LEDS    5       // Make up an arbitrary number of LEDs
#endif

#define MAX_COUNT   10

#define TIMING_DEBUG    1
#define PRINTF_DISABLE  0

//*****************************************************************************
//
// Globals
//
//*****************************************************************************
volatile uint32_t g_ui32TimerCount = 0;

//*****************************************************************************
//
// Function to initialize Timer A0 to interrupt every 1/4 second.
//
//*****************************************************************************
void
timer_init(void)
{
    am_hal_timer_config_t       TimerConfig;

    //
    // Set up TIMER0
    //
    am_hal_timer_default_config_set(&TimerConfig);
    TimerConfig.eFunction = AM_HAL_TIMER_FN_CONTINUOUS;
    TimerConfig.ui32Compare0 = 12000000;
    am_hal_timer_config(0, &TimerConfig);
    am_hal_timer_clear(0);

    //
    // Clear the timer Interrupt
    //
    am_hal_timer_interrupt_clear(AM_HAL_TIMER_MASK(0, AM_HAL_TIMER_COMPARE0));

} // timerA0_init()

//*****************************************************************************
//
// Timer Interrupt Service Routine (ISR)
//
//*****************************************************************************
void
am_ctimer_isr(void)
{
#if TIMING_DEBUG
        am_hal_gpio_state_write(AM_BSP_GPIO_IOM0_MISO, AM_HAL_GPIO_OUTPUT_SET);
#endif
    //
    // Clear TimerA0 Interrupt (write to clear).
    //
    am_hal_timer_interrupt_clear(AM_HAL_TIMER_MASK(0, AM_HAL_TIMER_COMPARE0));
    am_hal_timer_clear(0);

    //
    // Increment count and set limit based on the number of LEDs available.
    //
    g_ui32TimerCount++;

#if TIMING_DEBUG
        am_hal_gpio_state_write(AM_BSP_GPIO_IOM0_MISO, AM_HAL_GPIO_OUTPUT_CLEAR);
#endif
} // am_ctimer_isr()

//*****************************************************************************
//
// Main function.
//
//*****************************************************************************
int
main(void)
{

#if TIMING_DEBUG
    am_hal_gpio_pinconfig(AM_BSP_GPIO_IOM0_MOSI, am_hal_gpio_pincfg_output);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_IOM0_MISO, am_hal_gpio_pincfg_output);
#endif

    //
    // Configure the board for low power operation.
    //
    //am_bsp_low_power_init();

    //
    // Initialize the printf interface for ITM output
    //
    am_bsp_debug_printf_enable();

    //
    // Initialize the count to max so that the counting begins with 0
    // after the first trip to the ISR.
    //
    g_ui32TimerCount = 0;

    //
    // Clear the terminal and print the banner.
    //
    am_util_stdio_terminal_clear();
    am_util_stdio_printf("Binary Counter Example for %s\n", AM_HAL_DEVICE_NAME);
    am_util_stdio_printf("  (Timer clock source is HFRC)\n");

    //
    // We are done printing. Disable debug printf messages on ITM.
    //
#if PRINTF_DISABLE
    am_bsp_debug_printf_disable();
#endif

    //
    // Initialize the timer.
    //
    timer_init();

    //
    // Enable the timer Interrupt.
    //
    am_hal_timer_interrupt_enable(AM_HAL_TIMER_MASK(0, AM_HAL_TIMER_COMPARE0));

    //
    // Enable the timer interrupt in the NVIC.
    //
    NVIC_EnableIRQ(TIMER_IRQn);
    am_hal_interrupt_master_enable();

    //
    // Start timer A0
    //
    am_hal_timer_start(0);

    //
    // Loop forever.
    //
    while (1)
    {
        //
        // Go to Deep Sleep.
        //
        am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);

#if TIMING_DEBUG
        am_hal_gpio_state_write(AM_BSP_GPIO_IOM0_MOSI, AM_HAL_GPIO_OUTPUT_SET);
#endif
        //
        // Enable debug printf messages using ITM on SWO pin
        //
#if PRINTF_DISABLE
        am_bsp_debug_printf_enable();
#endif

        am_util_stdio_printf("%2d ", g_ui32TimerCount);

        if (g_ui32TimerCount >= MAX_COUNT)
        {
            g_ui32TimerCount = 0;
            am_util_stdio_printf("\n");
        }

        //am_hal_delay_us(10000);

        //
        // We are done printing. Disable debug printf messages on ITM.
        //
#if PRINTF_DISABLE
        am_bsp_debug_printf_disable();
#endif

#if TIMING_DEBUG
        am_hal_gpio_state_write(AM_BSP_GPIO_IOM0_MOSI, AM_HAL_GPIO_OUTPUT_CLEAR);
#endif
    }
} // main()
