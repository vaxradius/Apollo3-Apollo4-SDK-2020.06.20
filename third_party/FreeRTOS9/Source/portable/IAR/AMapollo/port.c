/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/*-----------------------------------------------------------
 * Implementation of functions defined in portable.h for the ARM CM4F port.
 *----------------------------------------------------------*/

/* Compiler includes. */
#include <intrinsics.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"

/* hardware includes */
#include "am_mcu_apollo.h"
#include "am_bsp.h"



// Check to make sure the FreeRTOSConfig.h options are consistent per the implementation
#if configOVERRIDE_DEFAULT_TICK_CONFIGURATION == 1
#if configUSE_TICKLESS_IDLE != 2
#error "configOVERRIDE_DEFAULT_TICK_CONFIGURATION == 1 supported only for configUSE_TICKLESS_IDLE = 2"
#endif

#ifndef AM_FREERTOS_USE_STIMER_FOR_TICK

// Determine which CTimer to use if configured to use CTimer for FreeRTOS Tick
#ifndef configCTIMER_NUM
// Default
#define configCTIMER_NUM 	3
#endif

#ifdef AM_PART_APOLLO
#if configCTIMER_NUM == 0
#error "Apollo can not use CTimer0 in 32b mode"
#endif
#endif

#if configCTIMER_NUM == 0
#define AM_FREERTOS_CTIMER_INT 	AM_HAL_CTIMER_INT_TIMERA0
#elif configCTIMER_NUM == 1
#define AM_FREERTOS_CTIMER_INT 	AM_HAL_CTIMER_INT_TIMERA1
#elif configCTIMER_NUM == 2
#define AM_FREERTOS_CTIMER_INT 	AM_HAL_CTIMER_INT_TIMERA2
#elif configCTIMER_NUM == 3
#define AM_FREERTOS_CTIMER_INT 	AM_HAL_CTIMER_INT_TIMERA3
#endif

#ifndef configCTIMER_CLOCK_HZ
// Default
#define configCTIMER_CLOCK_HZ   32768
#define configCTIMER_CLOCK      AM_HAL_CTIMER_XT_32_768KHZ
#else
#ifndef configCTIMER_CLOCK
#if configCTIMER_CLOCK_HZ == 32768
// Default - for backward compatibility
#define configCTIMER_CLOCK      AM_HAL_CTIMER_XT_32_768KHZ
#else
#error "configCTIMER_CLOCK not specified"
#endif
#endif
#endif

#else

#ifdef AM_PART_APOLLO
#error "Apollo can not use STimer for FreeRTOS"
#endif

#ifndef configSTIMER_CLOCK_HZ
// Default
#define configSTIMER_CLOCK_HZ   32768
#define configSTIMER_CLOCK      AM_HAL_STIMER_XTAL_32KHZ
#else
#ifndef configSTIMER_CLOCK
#if configSTIMER_CLOCK_HZ == 32768
// Default - for backward compatibility
#define configSTIMER_CLOCK      AM_HAL_STIMER_XTAL_32KHZ
#else
#error "configSTIMER_CLOCK not specified"
#endif
#endif
#endif

// Keeps the snapshot of the STimer corresponding to last tick update
static uint32_t g_lastSTimerVal = 0;
#endif

/* The Ctimer is a 16-bit counter.  */
#define portMAX_16_BIT_NUMBER		( 0x0000ffffUL )
/* The Stimer is a 32-bit counter. */
#define portMAX_32_BIT_NUMBER		( 0xffffffffUL )


#endif
#if configOVERRIDE_DEFAULT_TICK_CONFIGURATION == 0
#if configUSE_TICKLESS_IDLE == 2
// This implementation is TODO - will use Systick when active, but fall back to STimer/Ctimer when Idle
// Some crude analysis showed that doing so is no better than using CTImer/STimer always, in terms of power
// Hence there is no plan currently to implement it.
#error "configOVERRIDE_DEFAULT_TICK_CONFIGURATION == 0 not supported for configUSE_TICKLESS_IDLE = 2"
#endif
#endif
#ifndef __ARMVFP__
	#error This port can only be used when the project options are configured to enable hardware floating point support.
#endif

#if configMAX_SYSCALL_INTERRUPT_PRIORITY == 0
	#error configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to 0.  See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
#endif

#ifndef configSYSTICK_CLOCK_HZ
	#define configSYSTICK_CLOCK_HZ configCPU_CLOCK_HZ
	/* Ensure the SysTick is clocked at the same frequency as the core. */
	#define portNVIC_SYSTICK_CLK_BIT	( 1UL << 2UL )
#else
	/* The way the SysTick is clocked is not modified in case it is not the same
	as the core. */
	#define portNVIC_SYSTICK_CLK_BIT	( 0 )
#endif

/* Constants required to manipulate the core.  Registers first... */
#define portNVIC_SYSTICK_CTRL_REG			( * ( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG			( * ( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG	( * ( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SYSPRI2_REG				( * ( ( volatile uint32_t * ) 0xe000ed20 ) )
/* ...then bits in the registers. */
#define portNVIC_SYSTICK_INT_BIT			( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT			( 1UL << 0UL )
#define portNVIC_SYSTICK_COUNT_FLAG_BIT		( 1UL << 16UL )
#define portNVIC_PENDSVCLEAR_BIT 			( 1UL << 27UL )
#define portNVIC_PEND_SYSTICK_CLEAR_BIT		( 1UL << 25UL )

/* Constants used to detect a Cortex-M7 r0p1 core, which should use the ARM_CM7
r0p1 port. */
#define portCPUID							( * ( ( volatile uint32_t * ) 0xE000ed00 ) )
#define portCORTEX_M7_r0p1_ID				( 0x410FC271UL )
#define portCORTEX_M7_r0p0_ID				( 0x410FC270UL )

#define portNVIC_PENDSV_PRI					( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 16UL )
#define portNVIC_SYSTICK_PRI				( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 24UL )

/* Constants required to check the validity of an interrupt priority. */
#define portFIRST_USER_INTERRUPT_NUMBER		( 16 )
#define portNVIC_IP_REGISTERS_OFFSET_16 	( 0xE000E3F0 )
#define portAIRCR_REG						( * ( ( volatile uint32_t * ) 0xE000ED0C ) )
#define portMAX_8_BIT_VALUE					( ( uint8_t ) 0xff )
#define portTOP_BIT_OF_BYTE					( ( uint8_t ) 0x80 )
#define portMAX_PRIGROUP_BITS				( ( uint8_t ) 7 )
#define portPRIORITY_GROUP_MASK				( 0x07UL << 8UL )
#define portPRIGROUP_SHIFT					( 8UL )

/* Masks off all bits but the VECTACTIVE bits in the ICSR register. */
#define portVECTACTIVE_MASK					( 0xFFUL )

/* Constants required to manipulate the VFP. */
#define portFPCCR							( ( volatile uint32_t * ) 0xe000ef34 ) /* Floating point context control register. */
#define portASPEN_AND_LSPEN_BITS			( 0x3UL << 30UL )

/* Constants required to set up the initial stack. */
#define portINITIAL_XPSR					( 0x01000000 )
#define portINITIAL_EXEC_RETURN				( 0xfffffffd )

/* The systick is a 24-bit counter. */
#define portMAX_24_BIT_NUMBER				( 0xffffffUL )

/* A fiddle factor to estimate the number of SysTick counts that would have
occurred while the SysTick counter is stopped during tickless idle
calculations. */
#define portMISSED_COUNTS_FACTOR			( 45UL )

/* For strict compliance with the Cortex-M spec the task start address should
have bit-0 clear, as it is loaded into the PC on exit from an ISR. */
#define portSTART_ADDRESS_MASK				( ( StackType_t ) 0xfffffffeUL )

/* Each task maintains its own interrupt status in the critical nesting
variable. */
static UBaseType_t uxCriticalNesting = 0xaaaaaaaa;

/*
 * Setup the timer to generate the tick interrupts.  The implementation in this
 * file is weak to allow application writers to change the timer used to
 * generate the tick interrupt.
 */
void vPortSetupTimerInterrupt( void );

/*
 * Exception handlers.
 */
void xPortSysTickHandler( void );

/*
 * Start first task is a separate function so it can be tested in isolation.
 */
extern void vPortStartFirstTask( void );

/*
 * Turn the VFP on.
 */
extern void vPortEnableVFP( void );

/*
 * Used to catch tasks that attempt to return from their implementing function.
 */
static void prvTaskExitError( void );

/*-----------------------------------------------------------*/

/*
 * The number of SysTick increments that make up one tick period.
 */
#if configUSE_TICKLESS_IDLE == 1
	static uint32_t ulTimerCountsForOneTick = 0;
#endif /* configUSE_TICKLESS_IDLE */

/*
 * The maximum number of tick periods that can be suppressed is limited by the
 * 24 bit resolution of the SysTick timer.
 */
#if configUSE_TICKLESS_IDLE == 1
	static uint32_t xMaximumPossibleSuppressedTicks = 0;
#endif /* configUSE_TICKLESS_IDLE */

/*
 * Compensate for the CPU cycles that pass while the SysTick is stopped (low
 * power functionality only.
 */
#if configUSE_TICKLESS_IDLE == 1
	static uint32_t ulStoppedTimerCompensation = 0;
#endif /* configUSE_TICKLESS_IDLE */

/*
 * Used by the portASSERT_IF_INTERRUPT_PRIORITY_INVALID() macro to ensure
 * FreeRTOS API functions are not called from interrupts that have been assigned
 * a priority above configMAX_SYSCALL_INTERRUPT_PRIORITY.
 */
#if ( configASSERT_DEFINED == 1 )
	 static uint8_t ucMaxSysCallPriority = 0;
	 static uint32_t ulMaxPRIGROUPValue = 0;
	 static const volatile uint8_t * const pcInterruptPriorityRegisters = ( const volatile uint8_t * const ) portNVIC_IP_REGISTERS_OFFSET_16;
#endif /* configASSERT_DEFINED */

/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters )
{
	/* Simulate the stack frame as it would be created by a context switch
	interrupt. */

	/* Offset added to account for the way the MCU uses the stack on entry/exit
	of interrupts, and to ensure alignment. */
	pxTopOfStack--;

	*pxTopOfStack = portINITIAL_XPSR;	/* xPSR */
	pxTopOfStack--;
	*pxTopOfStack = ( ( StackType_t ) pxCode ) & portSTART_ADDRESS_MASK;	/* PC */
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) prvTaskExitError;	/* LR */

	/* Save code space by skipping register initialisation. */
	pxTopOfStack -= 5;	/* R12, R3, R2 and R1. */
	*pxTopOfStack = ( StackType_t ) pvParameters;	/* R0 */

	/* A save method is being used that requires each task to maintain its
	own exec return value. */
	pxTopOfStack--;
	*pxTopOfStack = portINITIAL_EXEC_RETURN;

	pxTopOfStack -= 8;	/* R11, R10, R9, R8, R7, R6, R5 and R4. */

	return pxTopOfStack;
}
/*-----------------------------------------------------------*/

static void prvTaskExitError( void )
{
	/* A function that implements a task must not exit or attempt to return to
	its caller as there is nothing to return to.  If a task wants to exit it
	should instead call vTaskDelete( NULL ).

	Artificially force an assert() to be triggered if configASSERT() is
	defined, then stop here so application writers can catch the error. */
	configASSERT( uxCriticalNesting == ~0UL );
	portDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
BaseType_t xPortStartScheduler( void )
{
	/* configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to 0.
	See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html */
	configASSERT( configMAX_SYSCALL_INTERRUPT_PRIORITY );

	/* This port can be used on all revisions of the Cortex-M7 core other than
	the r0p1 parts.  r0p1 parts should use the port from the
	/source/portable/GCC/ARM_CM7/r0p1 directory. */
	configASSERT( portCPUID != portCORTEX_M7_r0p1_ID );
	configASSERT( portCPUID != portCORTEX_M7_r0p0_ID );

	#if( configASSERT_DEFINED == 1 )
	{
		volatile uint32_t ulOriginalPriority;
		volatile uint8_t * const pucFirstUserPriorityRegister = ( volatile uint8_t * const ) ( portNVIC_IP_REGISTERS_OFFSET_16 + portFIRST_USER_INTERRUPT_NUMBER );
		volatile uint8_t ucMaxPriorityValue;

		/* Determine the maximum priority from which ISR safe FreeRTOS API
		functions can be called.  ISR safe functions are those that end in
		"FromISR".  FreeRTOS maintains separate thread and ISR API functions to
		ensure interrupt entry is as fast and simple as possible.

		Save the interrupt priority value that is about to be clobbered. */
		ulOriginalPriority = *pucFirstUserPriorityRegister;

		/* Determine the number of priority bits available.  First write to all
		possible bits. */
		*pucFirstUserPriorityRegister = portMAX_8_BIT_VALUE;

		/* Read the value back to see how many bits stuck. */
		ucMaxPriorityValue = *pucFirstUserPriorityRegister;

		/* Use the same mask on the maximum system call priority. */
		ucMaxSysCallPriority = configMAX_SYSCALL_INTERRUPT_PRIORITY & ucMaxPriorityValue;

		/* Calculate the maximum acceptable priority group value for the number
		of bits read back. */
		ulMaxPRIGROUPValue = portMAX_PRIGROUP_BITS;
		while( ( ucMaxPriorityValue & portTOP_BIT_OF_BYTE ) == portTOP_BIT_OF_BYTE )
		{
			ulMaxPRIGROUPValue--;
			ucMaxPriorityValue <<= ( uint8_t ) 0x01;
		}

		/* Shift the priority group value back to its position within the AIRCR
		register. */
		ulMaxPRIGROUPValue <<= portPRIGROUP_SHIFT;
		ulMaxPRIGROUPValue &= portPRIORITY_GROUP_MASK;

		/* Restore the clobbered interrupt priority register to its original
		value. */
		*pucFirstUserPriorityRegister = ulOriginalPriority;
	}
	#endif /* conifgASSERT_DEFINED */

	/* Make PendSV and SysTick the lowest priority interrupts. */
	portNVIC_SYSPRI2_REG |= portNVIC_PENDSV_PRI;
	portNVIC_SYSPRI2_REG |= portNVIC_SYSTICK_PRI;

	/* Start the timer that generates the tick ISR.  Interrupts are disabled
	here already. */
	vPortSetupTimerInterrupt();

	/* Initialise the critical nesting count ready for the first task. */
	uxCriticalNesting = 0;

	/* Ensure the VFP is enabled - it should be anyway. */
	vPortEnableVFP();

	/* Lazy save always. */
	*( portFPCCR ) |= portASPEN_AND_LSPEN_BITS;

	/* Start the first task. */
	vPortStartFirstTask();

	/* Should not get here! */
	return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
	/* Not implemented in ports where there is nothing to return to.
	Artificially force an assert. */
	configASSERT( uxCriticalNesting == 1000UL );
}
/*-----------------------------------------------------------*/

void vPortEnterCritical( void )
{
	portDISABLE_INTERRUPTS();
	uxCriticalNesting++;

	/* This is not the interrupt safe version of the enter critical function so
	assert() if it is being called from an interrupt context.  Only API
	functions that end in "FromISR" can be used in an interrupt.  Only assert if
	the critical nesting count is 1 to protect against recursive calls if the
	assert function also uses a critical section. */
	if( uxCriticalNesting == 1 )
	{
		configASSERT( ( portNVIC_INT_CTRL_REG & portVECTACTIVE_MASK ) == 0 );
	}
}
/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{
	configASSERT( uxCriticalNesting );
	uxCriticalNesting--;
	if( uxCriticalNesting == 0 )
	{
		portENABLE_INTERRUPTS();
	}
}
/*-----------------------------------------------------------*/

void xPortSysTickHandler( void )
{
	/* The SysTick runs at the lowest interrupt priority, so when this interrupt
	executes all interrupts must be unmasked.  There is therefore no need to
	save and then restore the interrupt mask value as its value is already
	known. */
	portDISABLE_INTERRUPTS();
	// Addition for support of SystemView Profiler
	traceISR_ENTER();
	// End addition
	{
		/* Increment the RTOS tick. */
		if( xTaskIncrementTick() != pdFALSE )
		{
		  // Addition for support of SystemView Profiler
		  traceISR_EXIT_TO_SCHEDULER();
		  // End addition

			/* A context switch is required.  Context switching is performed in
			the PendSV interrupt.  Pend the PendSV interrupt. */
			portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
		}
		// Addition for support of SystemView Profiler
		else
		{
		  traceISR_EXIT();
		}
		// End addition
	}
	portENABLE_INTERRUPTS();
}
/*-----------------------------------------------------------*/

#if configUSE_TICKLESS_IDLE == 1

	__weak void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
	{
	uint32_t ulReloadValue, ulCompleteTickPeriods, ulCompletedSysTickDecrements, ulSysTickCTRL;
	TickType_t xModifiableIdleTime;

		/* Make sure the SysTick reload value does not overflow the counter. */
		if( xExpectedIdleTime > xMaximumPossibleSuppressedTicks )
		{
			xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
		}

		/* Stop the SysTick momentarily.  The time the SysTick is stopped for
		is accounted for as best it can be, but using the tickless mode will
		inevitably result in some tiny drift of the time maintained by the
		kernel with respect to calendar time. */
		portNVIC_SYSTICK_CTRL_REG &= ~portNVIC_SYSTICK_ENABLE_BIT;

		/* Calculate the reload value required to wait xExpectedIdleTime
		tick periods.  -1 is used because this code will execute part way
		through one of the tick periods. */
		ulReloadValue = portNVIC_SYSTICK_CURRENT_VALUE_REG + ( ulTimerCountsForOneTick * ( xExpectedIdleTime - 1UL ) );
		if( ulReloadValue > ulStoppedTimerCompensation )
		{
			ulReloadValue -= ulStoppedTimerCompensation;
		}

		/* Enter a critical section but don't use the taskENTER_CRITICAL()
		method as that will mask interrupts that should exit sleep mode. */
		__disable_interrupt();
		__DSB();
		__ISB();


		/* If a context switch is pending or a task is waiting for the scheduler
		to be unsuspended then abandon the low power entry. */
		if( eTaskConfirmSleepModeStatus() == eAbortSleep )
		{
			/* Restart from whatever is left in the count register to complete
			this tick period. */
			portNVIC_SYSTICK_LOAD_REG = portNVIC_SYSTICK_CURRENT_VALUE_REG;

			/* Restart SysTick. */
			portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;

			/* Reset the reload register to the value required for normal tick
			periods. */
			portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1UL;

			/* Re-enable interrupts - see comments above __disable_interrupt()
			call above. */
			__enable_interrupt();
		}
		else
		{
			/* Set the new reload value. */
			portNVIC_SYSTICK_LOAD_REG = ulReloadValue;

			/* Clear the SysTick count flag and set the count value back to
			zero. */
			portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

			/* Restart SysTick. */
			portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;

			/* Sleep until something happens.  configPRE_SLEEP_PROCESSING() can
			set its parameter to 0 to indicate that its implementation contains
			its own wait for interrupt or wait for event instruction, and so wfi
			should not be executed again.  However, the original expected idle
			time variable must remain unmodified, so a copy is taken. */
			xModifiableIdleTime = xExpectedIdleTime;
			configPRE_SLEEP_PROCESSING( xModifiableIdleTime );
			if( xModifiableIdleTime > 0 )
			{
				__DSB();
				__WFI();
				__ISB();
			}
			configPOST_SLEEP_PROCESSING( xExpectedIdleTime );

			/* Stop SysTick.  Again, the time the SysTick is stopped for is
			accounted for as best it can be, but using the tickless mode will
			inevitably result in some tiny drift of the time maintained by the
			kernel with respect to calendar time. */
			ulSysTickCTRL = portNVIC_SYSTICK_CTRL_REG;
			portNVIC_SYSTICK_CTRL_REG = ( ulSysTickCTRL & ~portNVIC_SYSTICK_ENABLE_BIT );

			/* Re-enable interrupts - see comments above __disable_interrupt()
			call above. */
			__enable_interrupt();

			if( ( ulSysTickCTRL & portNVIC_SYSTICK_COUNT_FLAG_BIT ) != 0 )
			{
				uint32_t ulCalculatedLoadValue;

				/* The tick interrupt has already executed, and the SysTick
				count reloaded with ulReloadValue.  Reset the
				portNVIC_SYSTICK_LOAD_REG with whatever remains of this tick
				period. */
				ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL ) - ( ulReloadValue - portNVIC_SYSTICK_CURRENT_VALUE_REG );

				/* Don't allow a tiny value, or values that have somehow
				underflowed because the post sleep hook did something
				that took too long. */
				if( ( ulCalculatedLoadValue < ulStoppedTimerCompensation ) || ( ulCalculatedLoadValue > ulTimerCountsForOneTick ) )
				{
					ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL );
				}

				portNVIC_SYSTICK_LOAD_REG = ulCalculatedLoadValue;

				/* The tick interrupt handler will already have pended the tick
				processing in the kernel.  As the pending tick will be
				processed as soon as this function exits, the tick value
				maintained by the tick is stepped forward by one less than the
				time spent waiting. */
				ulCompleteTickPeriods = xExpectedIdleTime - 1UL;
			}
			else
			{
				/* Something other than the tick interrupt ended the sleep.
				Work out how long the sleep lasted rounded to complete tick
				periods (not the ulReload value which accounted for part
				ticks). */
				ulCompletedSysTickDecrements = ( xExpectedIdleTime * ulTimerCountsForOneTick ) - portNVIC_SYSTICK_CURRENT_VALUE_REG;

				/* How many complete tick periods passed while the processor
				was waiting? */
				ulCompleteTickPeriods = ulCompletedSysTickDecrements / ulTimerCountsForOneTick;

				/* The reload value is set to whatever fraction of a single tick
				period remains. */
				portNVIC_SYSTICK_LOAD_REG = ( ( ulCompleteTickPeriods + 1UL ) * ulTimerCountsForOneTick ) - ulCompletedSysTickDecrements;
			}

			/* Restart SysTick so it runs from portNVIC_SYSTICK_LOAD_REG
			again, then set portNVIC_SYSTICK_LOAD_REG back to its standard
			value.  The critical section is used to ensure the tick interrupt
			can only execute once in the case that the reload register is near
			zero. */
			portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;
			portENTER_CRITICAL();
			{
				portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;
				vTaskStepTick( ulCompleteTickPeriods );
				portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1UL;
			}
			portEXIT_CRITICAL();
		}
	}

#endif /* #if configUSE_TICKLESS_IDLE */
/*-----------------------------------------------------------*/

/*
 * Setup the systick timer to generate the tick interrupts at the required
 * frequency.
 */
#if configOVERRIDE_DEFAULT_TICK_CONFIGURATION == 0
__weak void vPortSetupTimerInterrupt( void )
{
	/* Calculate the constants required to configure the tick interrupt. */
	#if( configUSE_TICKLESS_IDLE == 1 )
	{
		ulTimerCountsForOneTick = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ );
		xMaximumPossibleSuppressedTicks = portMAX_24_BIT_NUMBER / ulTimerCountsForOneTick;
		ulStoppedTimerCompensation = portMISSED_COUNTS_FACTOR / ( configCPU_CLOCK_HZ / configSYSTICK_CLOCK_HZ );
	}
	#endif /* configUSE_TICKLESS_IDLE */

	/* Configure SysTick to interrupt at the requested rate. */
	portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
	portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT );
}
#endif /* configOVERRIDE_DEFAULT_TICK_CONFIGURATION */
/*-----------------------------------------------------------*/

#if( configASSERT_DEFINED == 1 )

	void vPortValidateInterruptPriority( void )
	{
	uint32_t ulCurrentInterrupt;
	uint8_t ucCurrentPriority;

		/* Obtain the number of the currently executing interrupt. */
		__asm volatile( "mrs %0, ipsr" : "=r"( ulCurrentInterrupt ) );

		/* Is the interrupt number a user defined interrupt? */
		if( ulCurrentInterrupt >= portFIRST_USER_INTERRUPT_NUMBER )
		{
			/* Look up the interrupt's priority. */
			ucCurrentPriority = pcInterruptPriorityRegisters[ ulCurrentInterrupt ];

			/* The following assertion will fail if a service routine (ISR) for
			an interrupt that has been assigned a priority above
			configMAX_SYSCALL_INTERRUPT_PRIORITY calls an ISR safe FreeRTOS API
			function.  ISR safe FreeRTOS API functions must *only* be called
			from interrupts that have been assigned a priority at or below
			configMAX_SYSCALL_INTERRUPT_PRIORITY.

			Numerically low interrupt priority numbers represent logically high
			interrupt priorities, therefore the priority of the interrupt must
			be set to a value equal to or numerically *higher* than
			configMAX_SYSCALL_INTERRUPT_PRIORITY.

			Interrupts that	use the FreeRTOS API must not be left at their
			default priority of	zero as that is the highest possible priority,
			which is guaranteed to be above configMAX_SYSCALL_INTERRUPT_PRIORITY,
			and	therefore also guaranteed to be invalid.

			FreeRTOS maintains separate thread and ISR API functions to ensure
			interrupt entry is as fast and simple as possible.

			The following links provide detailed information:
			http://www.freertos.org/RTOS-Cortex-M3-M4.html
			http://www.freertos.org/FAQHelp.html */
			configASSERT( ucCurrentPriority >= ucMaxSysCallPriority );
		}

		/* Priority grouping:  The interrupt controller (NVIC) allows the bits
		that define each interrupt's priority to be split between bits that
		define the interrupt's pre-emption priority bits and bits that define
		the interrupt's sub-priority.  For simplicity all bits must be defined
		to be pre-emption priority bits.  The following assertion will fail if
		this is not the case (if some bits represent a sub-priority).

		If the application only uses CMSIS libraries for interrupt
		configuration then the correct setting can be achieved on all Cortex-M
		devices by calling NVIC_SetPriorityGrouping( 0 ); before starting the
		scheduler.  Note however that some vendor specific peripheral libraries
		assume a non-zero priority group setting, in which cases using a value
		of zero will result in unpredicable behaviour. */
		configASSERT( ( portAIRCR_REG & portPRIORITY_GROUP_MASK ) <= ulMaxPRIGROUPValue );
	}

#endif /* configASSERT_DEFINED */

#if configOVERRIDE_DEFAULT_TICK_CONFIGURATION != 0
/*-----------------------------------------------------------
 * Implementation of functions defined in portable.h for the Ambiq Apollo_2 port.
 *----------------------------------------------------------*/
/* This port requires using the Stimer for Tickless_Idle in the Apollo_2 device  */      // dv**** 102616


#if configUSE_TICKLESS_IDLE == 2
	uint32_t ulTimerCountsForOneTick = 0;
/*
 * The maximum number of tick periods that can be suppressed is limited by the
 * resolution of the Tick timer.
 */
	static uint32_t xMaximumPossibleSuppressedTicks = 0;

void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
{
	uint32_t ulReloadValue;
    uint32_t New_Timer, Delta_Sleep;
	TickType_t xModifiableIdleTime;
    uint32_t elapsed_time;
	/* Make sure the SysTick reload value does not overflow the counter. */
	if( xExpectedIdleTime > xMaximumPossibleSuppressedTicks )
	{
		xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
	}


	/* Calculate the reload value required to wait xExpectedIdleTime
	tick periods.  -1 is used because this code will execute part way
	through one of the tick periods. */
	ulReloadValue =  ulTimerCountsForOneTick * ( xExpectedIdleTime - 1 );

	/* Enter a critical section but don't use the taskENTER_CRITICAL()
	method as that will mask interrupts that should exit sleep mode. */
    __disable_interrupt();
	__DSB();
	__ISB();

#ifdef AM_FREERTOS_USE_STIMER_FOR_TICK
    // Adjust for the time already elapsed
    elapsed_time = am_hal_stimer_counter_get() - g_lastSTimerVal;
#else
    am_hal_ctimer_stop(configCTIMER_NUM, AM_HAL_CTIMER_BOTH);
    // Adjust for the time already elapsed
    elapsed_time = am_hal_ctimer_read(configCTIMER_NUM, AM_HAL_CTIMER_BOTH);
#endif


	/* If a context switch is pending or a task is waiting for the scheduler
	to be unsuspended then abandon the low power entry. */
    /* Abandon low power entry if the sleep time is too short */
	if( (eTaskConfirmSleepModeStatus() == eAbortSleep) || ((elapsed_time + ulTimerCountsForOneTick) > ulReloadValue) )
	{
#ifndef AM_FREERTOS_USE_STIMER_FOR_TICK
        am_hal_ctimer_start(configCTIMER_NUM, AM_HAL_CTIMER_BOTH);
#endif
		/* Re-enable interrupts - see comments above __disable_irq() call
		above. */
		__enable_interrupt();

	}
	else
	{
        // Adjust for the time already elapsed
        ulReloadValue -= elapsed_time;
        // Initialize new timeout value
#ifdef AM_FREERTOS_USE_STIMER_FOR_TICK
        am_hal_stimer_compare_delta_set(0, ulReloadValue);
#else
        am_hal_ctimer_clear(configCTIMER_NUM, AM_HAL_CTIMER_BOTH);
        am_hal_ctimer_compare_set(configCTIMER_NUM, AM_HAL_CTIMER_BOTH, 0, ulReloadValue);
        am_hal_ctimer_start(configCTIMER_NUM, AM_HAL_CTIMER_BOTH);
#endif

		/* Sleep until something happens.  configPRE_SLEEP_PROCESSING() can
		set its parameter to 0 to indicate that its implementation contains
		its own wait for interrupt or wait for event instruction, and so wfi
		should not be executed again.  However, the original expected idle
		time variable must remain unmodified, so a copy is taken. */
		xModifiableIdleTime = xExpectedIdleTime;

		configPRE_SLEEP_PROCESSING( xModifiableIdleTime );       // Turn OFF all Periphials in this function

		if( xModifiableIdleTime > 0 )
		{
				__DSB();
				__WFI();
				__ISB();
		}

		configPOST_SLEEP_PROCESSING( xExpectedIdleTime );       // Turn ON all Periphials in this function

        // Any interrupt may have woken us up

        // Before renable interrupts, check how many ticks the processor has been in SLEEP
        // Adjust xTickCount via vTaskStepTick( Delta_Sleep )
        // to keep xTickCount up to date, as if ticks have been running all along

#ifdef AM_FREERTOS_USE_STIMER_FOR_TICK
        New_Timer = am_hal_stimer_counter_get();
        Delta_Sleep = (signed long) New_Timer - (signed long) g_lastSTimerVal;
        g_lastSTimerVal = New_Timer - Delta_Sleep%ulTimerCountsForOneTick;
#else
        am_hal_ctimer_stop(configCTIMER_NUM, AM_HAL_CTIMER_BOTH);
        New_Timer = am_hal_ctimer_read(configCTIMER_NUM, AM_HAL_CTIMER_BOTH);
        // INTSTAT check is needed to handle a possible case where the we came here without timer
        // incrementing at all....the value will still say 0, but it does not mean it expired
        if ((New_Timer == 0) && ((am_hal_ctimer_int_status_get(false) & (1 << configCTIMER_NUM*2))))
        {
            // The timer ran to completion and reset itself
            Delta_Sleep = ulReloadValue;
            // Clear the INTSTAT to prevent interrupt handler from counting an extra tick
            am_hal_ctimer_int_clear((1 << configCTIMER_NUM*2));
        } else
        {
            Delta_Sleep = New_Timer; // Indicates the time elapsed since we slept
        }
#endif

        Delta_Sleep /= ulTimerCountsForOneTick;

        // Correct System Tick after Sleep
        vTaskStepTick( Delta_Sleep );

		/* Restart System Tick */
#ifdef AM_FREERTOS_USE_STIMER_FOR_TICK

        // Clear the interrupt - to avoid extra tick counting in ISR
        am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREA);
        am_hal_stimer_compare_delta_set(0, ulTimerCountsForOneTick);
#else
        am_hal_ctimer_clear(configCTIMER_NUM, AM_HAL_CTIMER_BOTH);
        am_hal_ctimer_compare_set(configCTIMER_NUM, AM_HAL_CTIMER_BOTH, 0, ulTimerCountsForOneTick);


        am_hal_ctimer_start(configCTIMER_NUM, AM_HAL_CTIMER_BOTH);
#endif
        /* Re-enable interrupts - see comments above __disable_irq() call above. */
		__enable_interrupt();

	}
}

#endif /* #if configUSE_TICKLESS_IDLE = 2 */

#ifdef AM_FREERTOS_USE_STIMER_FOR_TICK

//*****************************************************************************
//
// Events associated with STimer CMP0 Interrupt
//
//  This is the FreeRTOS System Timer
//
//  Real Time events must be controlled by FreeRTOS as the Stimer is also used for sleep functions.
//  At any time the Stimer->Cmp0 interrupt can be a regular Tick interrupt or
//  an interrupt from a long Deep_Sleep().
//  The Deep Sleep in entered in Port.c->vPortSuppressTicksAndSleep() which is
//  entered from the IDLE task.
//  If no tasks are READY to run, vPortSuppressTicksAndSleep() is called.
//  See tasks.c-> portTASK_FUNCTION(...)
//
//
//
//
//*****************************************************************************
void
xPortStimerTickHandler(void)
{
    uint32_t remainder = 0;
    uint32_t curSTimer;
    uint32_t timerCounts;
    uint32_t numTicksElapsed;
    BaseType_t ctxtSwitchReqd = pdFALSE;

    curSTimer = am_hal_stimer_counter_get();
    //
    // Configure the STIMER->COMPARE_0
    //
    am_hal_stimer_compare_delta_set(0, ulTimerCountsForOneTick);

    timerCounts = curSTimer - g_lastSTimerVal;
    numTicksElapsed = timerCounts/ulTimerCountsForOneTick;
    remainder = timerCounts % ulTimerCountsForOneTick;
    g_lastSTimerVal = curSTimer - remainder;

    //
    // This is a timer a0 interrupt, perform the necessary functions
    // for the tick ISR.
    //
    (void) portSET_INTERRUPT_MASK_FROM_ISR();
	// Addition for support of SystemView Profiler
    traceISR_ENTER();
	// End addition
    {
        //
        // Increment RTOS tick
        // Allowing for need to increment the tick more than one... to avoid accumulation of
        // error in case of interrupt latencies
        //
        while (numTicksElapsed--)
        {
            ctxtSwitchReqd = (( xTaskIncrementTick() != pdFALSE ) ? pdTRUE : ctxtSwitchReqd);
        }
        if ( ctxtSwitchReqd != pdFALSE )
        {
	        // Addition for support of SystemView Profiler
        	traceISR_EXIT_TO_SCHEDULER();
			// End addition
            //
            // A context switch is required.  Context switching is
            // performed in the PendSV interrupt. Pend the PendSV
            // interrupt.
            //
            portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
        }
	    // Addition for support of SystemView Profiler
        else
        {
        	traceISR_EXIT();
        }
		// End addition
    }
    portCLEAR_INTERRUPT_MASK_FROM_ISR(0);
}


//*****************************************************************************
//
// Interrupt handler for the STIMER module Compare 0.
//
//*****************************************************************************
void
am_stimer_cmpr0_isr(void)
{

    //
    // Check the timer interrupt status.
    //
    uint32_t ui32Status = am_hal_stimer_int_status_get(false);
    if (ui32Status & AM_HAL_STIMER_INT_COMPAREA)
    {
        am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREA);

        //
        // Run handlers for the various possible timer events.
        //
        xPortStimerTickHandler();
    }
}



#else // Use CTimer
//*****************************************************************************
//
// Events associated with CTimer 0
//
//*****************************************************************************
void
xPortCTimer0TickHandler(void)
{
    // Restart the one-shot timer for next 'tick'
    am_hal_ctimer_clear(configCTIMER_NUM, AM_HAL_CTIMER_BOTH);
    am_hal_ctimer_compare_set(configCTIMER_NUM, AM_HAL_CTIMER_BOTH, 0, ulTimerCountsForOneTick);
    am_hal_ctimer_start(configCTIMER_NUM, AM_HAL_CTIMER_BOTH);
    //
    // This is a timer a0 interrupt, perform the necessary functions
    // for the tick ISR.
    //
    (void) portSET_INTERRUPT_MASK_FROM_ISR();

	// Addition for support of SystemView Profiler
    traceISR_ENTER();
	// End addition
    {
        //
        // Increment RTOS tick
        //
        if ( xTaskIncrementTick() != pdFALSE )
        {
	        // Addition for support of SystemView Profiler
        	traceISR_EXIT_TO_SCHEDULER();
			// End addition

            //
            // A context switch is required.  Context switching is
            // performed in the PendSV interrupt. Pend the PendSV
            // interrupt.
            //
            portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
        }
	    // Addition for support of SystemView Profiler
        else
        {
        	traceISR_EXIT();
        }
		// End addition
    }
    portCLEAR_INTERRUPT_MASK_FROM_ISR(0);

}


#endif // AM_FREERTOS_USE_STIMER_FOR_TICK


void vPortSetupTimerInterrupt( void )
{
#ifdef AM_FREERTOS_USE_STIMER_FOR_TICK
    uint32_t oldCfg;
    /* Calculate the constants required to configure the tick interrupt. */
    #if configUSE_TICKLESS_IDLE == 2
    {
        ulTimerCountsForOneTick = (configSTIMER_CLOCK_HZ /configTICK_RATE_HZ) ; //( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ );
        xMaximumPossibleSuppressedTicks = portMAX_32_BIT_NUMBER / ulTimerCountsForOneTick;
    }
    #endif /* configUSE_TICKLESS_IDLE */
    //
    //
    //
    am_hal_stimer_int_enable(AM_HAL_STIMER_INT_COMPAREA);

    //
    // Enable the timer interrupt in the NVIC, making sure to use the
    // appropriate priority level.
    //
#if AM_CMSIS_REGS
    NVIC_SetPriority(STIMER_CMPR0_IRQn, NVIC_configKERNEL_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(STIMER_CMPR0_IRQn);
#else // AM_CMSIS_REGS
    am_hal_interrupt_priority_set(AM_HAL_INTERRUPT_STIMER_CMPR0, configKERNEL_INTERRUPT_PRIORITY);
    am_hal_interrupt_enable(AM_HAL_INTERRUPT_STIMER_CMPR0);
#endif // AM_CMSIS_REGS

    //
    // Configure the STIMER
    //
    oldCfg = am_hal_stimer_config(AM_HAL_STIMER_CFG_FREEZE);
    g_lastSTimerVal = am_hal_stimer_counter_get();
    am_hal_stimer_compare_delta_set(0, ulTimerCountsForOneTick);
#if AM_CMSIS_REGS
    am_hal_stimer_config((oldCfg & ~(AM_HAL_STIMER_CFG_FREEZE | CTIMER_STCFG_CLKSEL_Msk)) | configSTIMER_CLOCK | AM_HAL_STIMER_CFG_COMPARE_A_ENABLE);
#else // AM_CMSIS_REGS
    am_hal_stimer_config((oldCfg & ~(AM_HAL_STIMER_CFG_FREEZE|AM_REG_CTIMER_STCFG_CLKSEL_M)) | configSTIMER_CLOCK | AM_HAL_STIMER_CFG_COMPARE_A_ENABLE);
#endif // AM_CMSIS_REGS

#else

    /* Calculate the constants required to configure the tick interrupt. */
    #if configUSE_TICKLESS_IDLE == 2
    {
        ulTimerCountsForOneTick = ( configCTIMER_CLOCK_HZ/configTICK_RATE_HZ) ;
        xMaximumPossibleSuppressedTicks = portMAX_32_BIT_NUMBER / ulTimerCountsForOneTick;
    }
    #endif /* configUSE_TICKLESS_IDLE */

    am_hal_ctimer_config_t cTimer0Config =
    {
        .ui32Link = 1,
        .ui32TimerAConfig = (configCTIMER_CLOCK |
                             AM_HAL_CTIMER_FN_ONCE|
                             AM_HAL_CTIMER_INT_ENABLE),

        .ui32TimerBConfig = 0
    };

    //
    // Configure the timer frequency and mode.
    //
    am_hal_ctimer_config(configCTIMER_NUM, &cTimer0Config);

    //
    // Set the timeout interval
    //
    am_hal_ctimer_compare_set(configCTIMER_NUM, AM_HAL_CTIMER_BOTH, 0, ulTimerCountsForOneTick);

    //
    // Enable the interrupt for timer A0
    //
    am_hal_ctimer_int_enable(AM_FREERTOS_CTIMER_INT);

    //
    // Enable the timer interrupt in the NVIC, making sure to use the
    // appropriate priority level.
    //
#if AM_CMSIS_REGS
    NVIC_SetPriority(CTIMER_IRQn, NVIC_configKERNEL_INTERRUPT_PRIORITY);
    am_hal_ctimer_int_register(AM_FREERTOS_CTIMER_INT, xPortCTimer0TickHandler);
    NVIC_EnableIRQ(CTIMER_IRQn);
#else // AM_CMSIS_REGS
    am_hal_interrupt_priority_set(AM_HAL_INTERRUPT_CTIMER, configKERNEL_INTERRUPT_PRIORITY);
    am_hal_ctimer_int_register(AM_FREERTOS_CTIMER_INT, xPortCTimer0TickHandler);
    am_hal_interrupt_enable(AM_HAL_INTERRUPT_CTIMER);
#endif // AM_CMSIS_REGS

    //
    // Enable the timer.
    //
    am_hal_ctimer_start(configCTIMER_NUM, AM_HAL_CTIMER_BOTH);


#endif // AM_FREERTOS_USE_STIMER_FOR_TICK
}


#endif /* configOVERRIDE_DEFAULT_TICK_CONFIGURATION */
