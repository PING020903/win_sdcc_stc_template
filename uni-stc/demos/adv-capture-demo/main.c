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
#include <advpwm-hal.h>
#include <timer-hal.h>

#ifdef ENABLE_PRINTF
	#include <uart-hal.h>
	#include <serial-console.h>
#endif // ENABLE_PRINTF

/*
 * Test setup ==========================================================
 * 
 * This test uses an STC8H1K28-36I-LQFP32.
 * An USB-to-serial adapter must be connected to P3.0 and P3.1.
 * A signal generator must be connected to P1.0.
 * 
 * The program will print the measured frequency and duty cycle to the 
 * serial console every second.
 * 
 * Start with a 1kHz signal with a 50% duty cycle, then vary the 
 * frequency (max. range: 2Hz-100kHz) and the duty cycle. The lower
 * the frequency, the better the accuracy.
 * =====================================================================
 */

// The maximum frequency we want to measure is 100kHz.
#define PWM_COUNTER_FREQ 100000ul
// We want the PWM counter to count from 0 to 65535, i.e. 65536 steps.
#define PWM_COUNTER_SCALE 65536ul

// Using a 10Hz timer tick works whatever MCU_FREQ.
#define PRINT_TIMER_FREQ 10ul
// We want to print the measurements every second.
#define PRINT_TIMER_TICKS (PRINT_TIMER_FREQ / 1)

#define NOW 0

static volatile uint8_t doPrint = PRINT_TIMER_TICKS;
// We'll need unsigned longs to calculate the frequency and duty cycle, 
// so let's store them accordingly from the start.
static volatile uint32_t lastPeriod = 0;
static volatile uint32_t lastUptime = 0;

void pwmOnChannelInterrupt(PWM_Channel channel, uint16_t HAL_PWM_SEGMENT counterValue) {
	switch (channel) {
	case PWM_Channel0:
		lastPeriod = ((uint32_t) counterValue);
		break;
	
	case PWM_Channel1:
		lastUptime = ((uint32_t) counterValue);
		break;
	}
}

INTERRUPT(timer2_isr, TIMER2_INTERRUPT) {
	if (doPrint) {
		doPrint--;
	}
}

void main() {
	INIT_EXTENDED_SFR()
	EA = 1;
	
#ifdef ENABLE_PRINTF
	serialConsoleInitialise(
		CONSOLE_UART, 
		CONSOLE_SPEED, 
		CONSOLE_PIN_CONFIG
	);
#endif // ENABLE_PRINTF
	
	// Configure capture ===============================================
	pwmConfigureCounter(
		PWM_COUNTER_A, 
		(uint16_t) (MCU_FREQ / PWM_COUNTER_FREQ - 1), 
		(uint16_t) (PWM_COUNTER_SCALE - 1),
		PWM_FREE_RUNNING, 
		PWM_NO_TRIGGER,
		0, // Repeat count
		PWM_BUFFERED_UPDATE,
		PWM_CONTINUOUS,
		PWM_EDGE_ALIGNED_UP,
		// IMPORTANT: the following 2 parameters are needed to record
		// the number of counter overflows, used to calculate elapsed
		// times in the PWM ISR.
		PWM_ENABLE_WRAP_UE_ONLY,
		ENABLE_INTERRUPT
	);
	
	// Channel 0 (P1.0 / pin 1) measures the frequency
	pwmInitialiseCapture(
		PWM_Channel0, 
		0, // Pin switch
		PWM_CAPTURE_ON_RISING_EDGE, 
		PWM_CAPTURE_SAME_PIN, 
		PWM_FILTER_1_CLOCK,
		PWM_REFERENCE_SAME_PIN
	);
	
	// Channel 1 measures the duty cycle
	pwmInitialiseCapture(
		PWM_Channel1, 
		0, // Pin switch
		PWM_CAPTURE_ON_FALLING_EDGE, 
		PWM_CAPTURE_OTHER_PIN, 
		PWM_FILTER_1_CLOCK,
		PWM_REFERENCE_OTHER_PIN
	);
	
	pwmEnableCounter(PWM_COUNTER_A);
	
	// Configure print timer ===========================================
	startTimer(
		TIMER2, 
		frequencyToSysclkDivisor(PRINT_TIMER_FREQ),
		DISABLE_OUTPUT, 
		ENABLE_INTERRUPT, 
		FREE_RUNNING
	);
	
	// Main loop -------------------------------------------------------
	
	while (1) {
		if (doPrint == NOW) {
			doPrint = PRINT_TIMER_TICKS;
			uint16_t frequency = PWM_COUNTER_FREQ / lastPeriod;
			uint16_t dutyCycle = lastUptime * 100ul / lastPeriod;
			printf("f=%uHz, d=%u%%\n", frequency, dutyCycle);
		}
	}
}
