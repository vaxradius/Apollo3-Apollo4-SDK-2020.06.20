//*****************************************************************************
//
//! @file deepsleep_wake.c
//!
//! @brief Example that goes to deepsleep and wakes from either the RTC or GPIO.
//!
//! Purpose: This example configures the device to go into a deep sleep mode. Once in
//! deep sleep the RTC peripheral will wake the device every second, check to
//! see if 5 seconds has elapsed and then toggle LED1.
//!
//! Alternatively, it will awake when button 0 is pressed and toggle LED0.
//!
//! The example begins by printing out a banner annoucement message through
//! the UART, which is then completely disabled for the remainder of execution.
//!
//! Text is output to the UART at 115,200 BAUD, 8 bit, no parity.
//! Please note that text end-of-line is a newline (LF) character only.
//! Therefore, the UART terminal must be set to simulate a CR/LF.
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

// RTC time structure.
am_hal_rtc_time_t g_sTime;


#ifdef APOLLO4_EVB
//*****************************************************************************
//
// BUTTON0 pin configuration settings.
//
//*****************************************************************************
//
// Set up the configuration for BUTTON0.
//
const am_hal_gpio_pincfg_t g_deepsleep_button0 =
{
    .uFuncSel = 3,
    .eIntDir = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,
};

//*****************************************************************************
//
// GPIO ISR
//
//*****************************************************************************
void
am_gpio_isr(void)
{
    //
    // Delay for debounce.
    //
    am_util_delay_ms(200);

    //
    // Clear the GPIO Interrupt (write to clear).
    //
    AM_HAL_GPIO_MASKCREATE(GpioIntMask);
    am_hal_gpio_interrupt_clear(AM_HAL_GPIO_MASKBIT(pGpioIntMask, AM_BSP_GPIO_BUTTON0));

    //
    // Toggle LED 0.
    //
#if (AM_BSP_NUM_LEDS > 0)
    am_devices_led_toggle(am_bsp_psLEDs, 0);
#endif
}
#endif

//*****************************************************************************
//
// RTC ISR
//
//*****************************************************************************
static uint32_t g_RTCseconds = 0;

void
am_rtc_isr(void)
{
    //
    // Clear the RTC alarm interrupt.
    //
    am_hal_rtc_interrupt_clear(AM_HAL_RTC_INT_ALM);

    //
    // Check the desired number of seconds until LED is toggled.
    //
    if ( ++g_RTCseconds >= 5 )
    {
        //
        // Reset the seconds counter.
        //
        g_RTCseconds = 0;

#if (AM_BSP_NUM_LEDS > 0)
        //
        // Toggle LED 1.
        //
        am_devices_led_toggle(am_bsp_psLEDs, 1);
#endif
    }
}

//*****************************************************************************
//
// Main function.
//
//*****************************************************************************
int
main(void)
{
    am_hal_pwrctrl_mcu_memory_config_t McuMemCfg =
    {
      .eCacheCfg    = AM_HAL_PWRCTRL_CACHE_NONE,
      .bRetainCache = false,
      .eDTCMCfg     = AM_HAL_PWRCTRL_DTCM_8K,
      .eRetainDTCM  = AM_HAL_PWRCTRL_DTCM_8K,
      .bEnableNVM0  = true,
      .bRetainNVM0  = true
    };

    am_hal_pwrctrl_sram_memcfg_t SRAMMemCfg =
    {
      .eSRAMCfg         = AM_HAL_PWRCTRL_SRAM_NONE,
      .eActiveWithMCU   = AM_HAL_PWRCTRL_SRAM_NONE,
      .eActiveWithDSP   = AM_HAL_PWRCTRL_SRAM_NONE,
      .eSRAMRetain      = AM_HAL_PWRCTRL_SRAM_NONE
    };

    //
    // Set the default cache configuration
    //
    am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
    am_hal_cachectrl_enable();

    //
    // Configure the board for low power operation.
    //
    am_bsp_low_power_init();

    //
    // Enable the XT for the RTC.
    //
    am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_XTAL_START, 0);

    //
    // Select XT for RTC clock source
    //
    am_hal_rtc_osc_select(AM_HAL_RTC_OSC_XT);

    //
    // Enable the RTC.
    //
    am_hal_rtc_osc_enable();

    //
    // Initialize the printf interface for UART output.
    //
    am_bsp_uart_printf_enable();

    //
    // Print the banner.
    //
    am_util_stdio_terminal_clear();
    am_util_stdio_printf("Deepsleep Wake Example\n");

    //
    // We are done printing.
    // Disable the UART and interrupts
    //
    am_bsp_uart_printf_disable();


    //
    // Update memory configuration to minimum.
    //
    am_hal_pwrctrl_mcu_memory_config(&McuMemCfg);
    am_hal_pwrctrl_sram_config(&SRAMMemCfg);

#if (AM_BSP_NUM_BUTTONS > 0)  &&  (AM_BSP_NUM_LEDS > 0)
    //
    // Configure the button pin.
    //
    am_hal_gpio_pinconfig(AM_BSP_GPIO_BUTTON0, g_deepsleep_button0);

    //
    // Clear the GPIO Interrupt (write to clear).
    //
    AM_HAL_GPIO_MASKCREATE(GpioIntMask);
    am_hal_gpio_interrupt_clear(AM_HAL_GPIO_MASKBIT(pGpioIntMask, AM_BSP_GPIO_BUTTON0));

    //
    // Enable the GPIO/button interrupt.
    //
    am_hal_gpio_interrupt_enable(AM_HAL_GPIO_MASKBIT(pGpioIntMask, AM_BSP_GPIO_BUTTON0));

    //
    // Configure the LEDs.
    //
    am_devices_led_array_init(am_bsp_psLEDs, AM_BSP_NUM_LEDS);

    //
    // Turn the LEDs off, but initialize LED1 on so user will see something.
    //
    for (int ix = 0; ix < AM_BSP_NUM_LEDS; ix++)
    {
        am_devices_led_off(am_bsp_psLEDs, ix);
    }

    am_devices_led_on(am_bsp_psLEDs, 1);
#endif // defined(AM_BSP_NUM_BUTTONS)  &&  defined(AM_BSP_NUM_LEDS)

    //
    // Set the alarm repeat interval to be every second.
    //
    am_hal_rtc_alarm_interval_set(AM_HAL_RTC_ALM_RPT_SEC);

    //
    // Clear the RTC alarm interrupt.
    //
    am_hal_rtc_interrupt_clear(AM_HAL_RTC_INT_ALM);

    //
    // Enable the RTC alarm interrupt.
    //
    am_hal_rtc_interrupt_enable(AM_HAL_RTC_INT_ALM);

    //
    // Enable GPIO interrupts to the NVIC.
    //
#ifdef APOLLO4_EVB
    NVIC_EnableIRQ(GPIO_IRQn);
#endif
    NVIC_EnableIRQ(RTC_IRQn);

    //
    // Enable interrupts to the core.
    //
    am_hal_interrupt_master_enable();

    while (1)
    {
        //
        // Go to Deep Sleep.
        //
        am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
    }
}
