/*
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Copyright (c) 2022 Vincent DEFERT. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the 
 * documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "project-defs.h"
#include <timer-hal.h>

/**
 * @file timer-hal.c
 * 
 * Timer abstraction implementation for STC12, STC15 and STC8.
 */

#if MCU_FAMILY == 12
	#define COUNTER_MAX 256UL
#else
	#define COUNTER_MAX 65536UL
#endif // MCU_FAMILY == 12

// Prescalers are 8-bit counters.
#define PRESCALER_MAX 256

#if MCU_FAMILY == 90
	#pragma save
	// Suppress warning "unreferenced function argument"
	#pragma disable_warning 85
#endif

TimerStatus startTimer(Timer timer, uint32_t sysclkDivisor, OutputEnable enableOutput, InterruptEnable enableInterrupt, CounterControl timerControl) {
	TimerStatus rc = TIMER_FREQUENCY_OK;
	bool sysclkDiv12 = false;
	
	if (sysclkDivisor == 0) {
		// Too high
		rc = TIMER_FREQUENCY_TOO_HIGH;
	} else {
#ifdef TIMER_HAS_PRESCALERS
		uint16_t prescaler = 0;
		
		switch (timer) {
		case TIMER0:
#ifdef TIMER_HAS_T1
		case TIMER1:
#endif // TIMER_HAS_T1
#endif // TIMER_HAS_PRESCALERS

			if (sysclkDivisor > COUNTER_MAX) {
				if (sysclkDivisor <= (COUNTER_MAX * 12UL)) {
					sysclkDiv12 = true;
					sysclkDivisor /= 12;
				} else {
					// Too low: T0 and T1 don't have a prescaler.
					rc = TIMER_FREQUENCY_TOO_LOW;
				}
			}

#ifdef TIMER_HAS_PRESCALERS
			break;
		
		default:
			if (sysclkDivisor > COUNTER_MAX) {
				prescaler = sysclkDivisor / COUNTER_MAX;
				
				if (prescaler > PRESCALER_MAX) {
					sysclkDiv12 = true;
					prescaler /= 12;
					sysclkDivisor /= 12;
				}
				
				// Round the result
				if (sysclkDivisor % prescaler) {
					prescaler++;
				}
				
				sysclkDivisor /= prescaler;
				
				// sysclk is divided by (TMxPS + 1)
				prescaler--;
			}
			break;
		}
		
		switch (timer) {
#ifdef TIMER_HAS_T2
		case TIMER2:
			TM2PS = prescaler;
			break;
#endif // TIMER_HAS_T2

#ifdef TIMER_HAS_T3_T4
		case TIMER3:
			TM3PS = prescaler;
			break;
		
		case TIMER4:
			TM4PS = prescaler;
			break;
#endif // TIMER_HAS_T3_T4

#ifdef TIMER_HAS_T11
		case TIMER11:
			TM11PS = prescaler;
			break;
#endif // TIMER_HAS_T11
		}
#endif // TIMER_HAS_PRESCALERS
	}

	if (rc == TIMER_FREQUENCY_OK) {
		uint16_t reloadValue = (uint16_t) (COUNTER_MAX - sysclkDivisor);
		
		switch (timer) {
		case TIMER0:
#ifdef M_T0x12
			// Set prescaler
			if (sysclkDiv12) {
				AUXR &= ~M_T0x12;
			} else {
				AUXR |= M_T0x12;
			}
#endif
			
			// Configure T0 as a 16-bit auto-reload timer (mode 0).
			TMOD &= 0xf0;
			
			// Configure timer control
			if (timerControl == FREE_RUNNING) {
				TMOD &= ~M_T0_GATE;
			} else {
				TMOD |= M_T0_GATE;
			}
			
#if MCU_FAMILY == 12
			// On the STC12, we'll use mode 2 (8-bit auto-reload timer).
			TMOD |= 2;
			
			if (enableOutput == DISABLE_OUTPUT) {
				WAKE_CLKO &= ~M_T0CLKO;
			} else {
				WAKE_CLKO |= M_T0CLKO;
			}
			
			T0H = T0L = reloadValue;
#elif MCU_FAMILY != 90
			if (enableOutput == DISABLE_OUTPUT) {
				INT_CLKO &= ~M_T0CLKO;
			} else {
				INT_CLKO |= M_T0CLKO;
			}
			
			T0 = reloadValue;
#endif
			
			if (enableInterrupt == DISABLE_INTERRUPT) {
				IE1 &= ~M_T0IE;
			} else {
				IE1 |= M_T0IE;
			}
			
			// Start timer
			TCON |= M_T0RUN;
			break;
		
#ifdef TIMER_HAS_T1
		case TIMER1:
	#if MCU_FAMILY == 12
			// Configure T1 as an 8-bit auto-reload timer (mode 2)
			TMOD = (TMOD & 0x0f) | (2 << P_T1_MODE);
			
			if (enableOutput == DISABLE_OUTPUT) {
				WAKE_CLKO &= ~M_T1CLKO;
			} else {
				WAKE_CLKO |= M_T1CLKO;
			}
			
			// Set reload value
			T1H = T1L = reloadValue;
	#else
			// Configure T1 as a 16-bit auto-reload timer (mode 0)
			TMOD &= 0x0f;
			
		#if MCU_FAMILY != 90
			if (enableOutput == DISABLE_OUTPUT) {
				INT_CLKO &= ~M_T1CLKO;
			} else {
				INT_CLKO |= M_T1CLKO;
			}
		#endif
			
			// Set reload value
			T1 = reloadValue;
	#endif // MCU_FAMILY == 12
			
			// Configure timer control
			if (timerControl == FREE_RUNNING) {
				TMOD &= ~M_T1_GATE;
			} else {
				TMOD |= M_T1_GATE;
			}

	#ifdef M_T1x12
			// Configure prescaling
			if (sysclkDiv12) {
				AUXR &= ~M_T1x12;
			} else {
				AUXR |= M_T1x12;
			}
	#endif
			
			if (enableInterrupt == DISABLE_INTERRUPT) {
				IE1 &= ~M_T1IE;
			} else {
				IE1 |= M_T1IE;
			}
			
			// Start timer
			TCON |= M_T1RUN;
			break;
#endif // TIMER_HAS_T1

#if defined(TIMER_HAS_T2) || defined(TIMER_HAS_BRT)
		case TIMER2:
#ifdef TIMER_HAS_T2
			// Configure T2 in timer mode
			AUXR &= ~M_T2_C_T;
			
			if (enableOutput == DISABLE_OUTPUT) {
				INT_CLKO &= ~M_T2CLKO;
			} else {
				INT_CLKO |= M_T2CLKO;
			}
			
			if (enableInterrupt == DISABLE_INTERRUPT) {
				IE2 &= ~M_T2IE;
			} else {
				IE2 |= M_T2IE;
			}
			
			// Set reload value
			T2 = reloadValue;
#endif // TIMER_HAS_T2

#ifdef TIMER_HAS_BRT
			if (enableOutput == DISABLE_OUTPUT) {
				WAKE_CLKO &= ~M_BRTCLKO;
			} else {
				WAKE_CLKO |= M_BRTCLKO;
			}
			
			// Set reload value
			BRT =  reloadValue;
#endif // TIMER_HAS_BRT

			// Configure prescaling
			if (sysclkDiv12) {
				AUXR &= ~M_T2x12;
			} else {
				AUXR |= M_T2x12;
			}
			
			// Start timer
			AUXR |= M_T2RUN;
			break;
#endif // defined(TIMER_HAS_T2) || defined(TIMER_HAS_BRT)
	
#ifdef TIMER_HAS_T3_T4
		case TIMER3:
			// Configure T3 in timer mode
			T4T3M &= ~M_T3_C_T;
			
			if (enableOutput == DISABLE_OUTPUT) {
				T4T3M &= ~M_T3CLKO;
			} else {
				T4T3M |= M_T3CLKO;
			}
			
			if (enableInterrupt == DISABLE_INTERRUPT) {
				IE2 &= ~M_T3IE;
			} else {
				IE2 |= M_T3IE;
			}
			
			// Configure prescaling
			if (sysclkDiv12) {
				T4T3M &= ~M_T3x12;
			} else {
				T4T3M |= M_T3x12;
			}
			
			// Set reload value
			T3 = reloadValue;
			
			// Start timer
			T4T3M |= M_T3RUN;
			break;
		
		case TIMER4:
			// Configure T4 in timer mode
			T4T3M &= ~M_T4_C_T;
			
			if (enableOutput == DISABLE_OUTPUT) {
				T4T3M &= ~M_T4CLKO;
			} else {
				T4T3M |= M_T4CLKO;
			}
			
			if (enableInterrupt == DISABLE_INTERRUPT) {
				IE2 &= ~M_T4IE;
			} else {
				IE2 |= M_T4IE;
			}
			
			// Configure prescaling
			if (sysclkDiv12) {
				T4T3M &= ~M_T4x12;
			} else {
				T4T3M |= M_T4x12;
			}
			
			// Set reload value
			T4 = reloadValue;
			
			// Start timer
			T4T3M |= M_T4RUN;
			break;
#endif // TIMER_HAS_T3_T4

#ifdef TIMER_HAS_T11
		case TIMER11:
			// Configure T11 in timer mode
			T11CR &= ~M_T11_C_T;
			
			if (enableOutput == DISABLE_OUTPUT) {
				T11CR &= ~M_T11CLKO;
			} else {
				T11CR |= M_T11CLKO;
			}
			
			if (enableInterrupt == DISABLE_INTERRUPT) {
				T11CR &= ~M_T11IE;
			} else {
				T11CR |= M_T11IE;
			}
			
			// Configure prescaling
			if (sysclkDiv12) {
				T11CR &= ~M_T11x12;
			} else {
				T11CR |= M_T11x12;
			}
			
			// Set reload value
			T11L = reloadValue;
			T11H = reloadValue >> 8;
			
			// Start timer
			T11CR |= M_T11RUN;
			break;
#endif // TIMER_HAS_T11
		}
	}
	
	return rc;
}

#if MCU_FAMILY == 90
	#pragma restore
#endif

#ifdef HAL_TIMER_API_STOP_TIMER

uint16_t stopTimer(Timer timer) {
	uint16_t counterValue = 0;
	
	switch (timer) {
	case TIMER0:
		// Stop timer
		TCON &= ~M_T0RUN;
		// Read counter value
#if MCU_FAMILY == 12
		counterValue = T0L;
#else
		counterValue = T0;
#endif
		// Disable timer output
#if MCU_FAMILY == 12
		WAKE_CLKO &= ~M_T0CLKO;
#else
		INT_CLKO &= ~M_T0CLKO;
#endif
		break;
	
#ifdef TIMER_HAS_T1
	case TIMER1:
		// Stop timer
		TCON &= ~M_T1RUN;
		// Read counter value
#if MCU_FAMILY == 12
		counterValue = T1L;
#else
		counterValue = T1;
#endif
		// Disable timer output
#if MCU_FAMILY == 12
		WAKE_CLKO &= ~M_T1CLKO;
#else
		INT_CLKO &= ~M_T1CLKO;
#endif
		break;
#endif // TIMER_HAS_T1

#if defined(TIMER_HAS_T2) || defined(TIMER_HAS_BRT)
	case TIMER2:
		// Stop timer
		AUXR &= ~M_T2RUN;
		// Read counter value
#ifdef TIMER_HAS_T2
		counterValue = T2;
#endif
#ifdef TIMER_HAS_BRT
		counterValue = BRT;
#endif
		// Disable timer output
#ifdef TIMER_HAS_T2
		INT_CLKO &= ~M_T2CLKO;
#endif
#ifdef TIMER_HAS_BRT
		WAKE_CLKO &= ~M_BRTCLKO;
#endif
		break;
#endif // defined(TIMER_HAS_T2) || defined(TIMER_HAS_BRT)

#ifdef TIMER_HAS_T3_T4
	case TIMER3:
		// Stop timer
		T4T3M &= ~M_T3RUN;
		// Read counter value
		counterValue = T3;
		// Disable timer output
		T4T3M &= ~M_T3CLKO;
		break;
	
	case TIMER4:
		// Stop timer
		T4T3M &= ~M_T4RUN;
		// Read counter value
		counterValue = T4;
		// Disable timer output
		T4T3M &= ~M_T4CLKO;
		break;
#endif // TIMER_HAS_T3_T4

#ifdef TIMER_HAS_T11
	case TIMER11:
		// Stop timer
		T11CR &= ~M_T11RUN;
		// Read counter value
		counterValue = (T11H << 8) | T11L;
		// Disable timer output
		T11CR &= ~M_T11CLKO;
		break;
#endif // TIMER_HAS_T11
	}
	
	return counterValue;
}

#endif // HAL_TIMER_API_STOP_TIMER

#ifdef HAL_TIMER_API_TIMER_PINS

static const uint8_t _pinConfigurations[][4] = {
#if MCU_FAMILY == 8
	#if MCU_PINS > 8
		// TIMER0
		{ 0x34, 0x35, 0xff, 0xff, }, 
		
		// TIMER1
		#ifdef TIMER_HAS_T1
			{ 0x35, 0x34, 0xff, 0xff, }, 
		#else  // TIMER_HAS_T1
			{ 0xff, 0xff, 0xff, 0xff, }, 
		#endif // TIMER_HAS_T1
		
		// TIMER2
		#ifdef TIMER_HAS_T2
			#ifdef GPIO_NO_P12
				{ 0x54, 0x13, 0xff, 0xff, }, 
			#else  // GPIO_NO_P12
				#ifdef GPIO_NO_P13
					{ 0x12, 0xff, 0xff, 0xff, }, 
				#else  // GPIO_NO_P12
					{ 0x12, 0x13, 0xff, 0xff, }, 
				#endif // GPIO_NO_P12
			#endif // GPIO_NO_P12
		#else  // TIMER_HAS_T2
			{ 0xff, 0xff, 0xff, 0xff, }, 
		#endif // TIMER_HAS_T2
		
		// TIMER3 & 4
		#ifdef TIMER_HAS_T3_T4
			#ifdef TIMER_HAS_T3_T4_PIN_SWITCH
				#if MCU_PINS > 32
					{ 0x04, 0x05, 0x00, 0x01, }, 
					{ 0x06, 0x07, 0x02, 0x03, }, 
				#else  // MCU_PINS > 32
					{ 0xff, 0xff, 0x00, 0x01, }, 
					{ 0xff, 0xff, 0x02, 0x03, }, 
				#endif // MCU_PINS > 32
			#else  // TIMER_HAS_T3_T4_PIN_SWITCH
				{ 0x00, 0x01, 0xff, 0xff, }, 
				{ 0x02, 0x03, 0xff, 0xff, }, 
			#endif // TIMER_HAS_T3_T4_PIN_SWITCH
		#else  // TIMER_HAS_T3_T4
			{ 0xff, 0xff, 0xff, 0xff, }, 
			{ 0xff, 0xff, 0xff, 0xff, }, 
		#endif // TIMER_HAS_T3_T4
		
		// TIMER11
		#ifdef TIMER_HAS_T11
			{ 0x14, 0x15, 0xff, 0xff, }, 
		#else  // TIMER_HAS_T11
			{ 0xff, 0xff, 0xff, 0xff, }, 
		#endif // TIMER_HAS_T11
	#else  // MCU_PINS > 8
		// TIMER0
		{ 0x54, 0x55, 0xff, 0xff, }, 
		#ifdef TIMER_HAS_T1
			// TIMER1
			{ 0x55, 0x54, 0xff, 0xff, }, 
		#else  // TIMER_HAS_T1
			{ 0xff, 0xff, 0xff, 0xff, }, 
		#endif // TIMER_HAS_T1
	#endif // MCU_PINS > 8
#endif // MCU_FAMILY == 8

#if MCU_FAMILY == 15
	// TIMER0
	#ifdef GPIO_NO_P34
		#ifdef TIMER0_ON_P1
			{ 0x12, 0x14, 0xff, 0xff, }, 
		#else  // TIMER0_ON_P1
			{ 0xff, 0xff, 0xff, 0xff, }, 
		#endif // TIMER0_ON_P1
	#else  // GPIO_NO_P34
		{ 0x34, 0x35, 0xff, 0xff, }, 
	#endif // GPIO_NO_P34
	
	// TIMER1
	#ifdef TIMER_HAS_T1
		{ 0x35, 0x34, 0xff, 0xff, }, 
	#else  // TIMER_HAS_T1
		{ 0xff, 0xff, 0xff, 0xff, }, 
	#endif // TIMER_HAS_T1
	
	// TIMER2
	{ 0x31, 0x30, 0xff, 0xff, }, 
	
	// TIMER3 & 4
	#ifdef TIMER_HAS_T3_T4
		#if MCU_PINS > 32
			{ 0x05, 0x04, 0xff, 0xff, }, 
			{ 0x07, 0x06, 0xff, 0xff, }, 
		#else  // MCU_PINS > 32
			{ 0xff, 0xff, 0xff, 0xff, }, 
			{ 0xff, 0xff, 0xff, 0xff, }, 
		#endif // MCU_PINS > 32
	#endif // TIMER_HAS_T3_T4
#endif // MCU_FAMILY == 15

#if MCU_FAMILY == 12
	// TIMER0
	{ 0x34, 0x35, 0xff, 0xff, }, 
	// TIMER1
	{ 0x35, 0x34, 0xff, 0xff, }, 
	// BRT
	{ 0xff, 0x10, 0xff, 0xff, }, 
#endif // MCU_FAMILY == 12
};

#pragma save
// Suppress warning "unreferenced function argument"
#pragma disable_warning 85
uint8_t getTimerPin(Timer timer, TimerPin pin, uint8_t pinSwitch) {
#ifdef TIMER_HAS_T3_T4_PIN_SWITCH
	return _pinConfigurations[timer][pin + (pinSwitch ? 2 : 0)];
#else  // TIMER_HAS_T3_T4_PIN_SWITCH
	return _pinConfigurations[timer][pin];
#endif // TIMER_HAS_T3_T4_PIN_SWITCH
}
#pragma restore

#endif // HAL_TIMER_API_TIMER_PINS
