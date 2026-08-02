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
#include <gpio-hal.h>
#include <timer-hal.h>

#ifdef ENABLE_PRINTF
	#include <uart-hal.h>
	#include <serial-console.h>
#endif // ENABLE_PRINTF

#ifdef MCU_HAS_PCA
	#include "glow-pca.h"
#endif // MCU_HAS_PCA

#ifdef MCU_HAS_ENHANCED_PWM
	#include "glow-enhpwm.h"
#endif // MCU_HAS_ENHANCED_PWM

#ifdef MCU_HAS_ADVANCED_PWM
	#include "glow-advpwm.h"
#endif // MCU_HAS_ADVANCED_PWM

static GpioConfig _blinkPin = GPIO_PIN_CONFIG(GPIO_PORT3, BLINK_PIN, GPIO_PUSH_PULL_MODE);
static uint8_t _blinkState = 0;

static uint16_t _glowCount = LED_GLOW_COUNT;
static uint16_t _blinkCount = LED_BLINK_COUNT;

INTERRUPT(timer0_isr, TIMER0_INTERRUPT) {
	if (_blinkCount) {
		_blinkCount--;
	}
	
	if (_glowCount) {
		_glowCount--;
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
	
	gpioConfigure(&_blinkPin);
	
	startTimer(
		TIMER0, 
		frequencyToSysclkDivisor(TIMER0_FREQ), 
		DISABLE_OUTPUT, 
		ENABLE_INTERRUPT, 
		FREE_RUNNING
	);
	
#ifdef MCU_HAS_PCA
	pcaGlowInitialise();
#endif // MCU_HAS_PCA

#ifdef MCU_HAS_ENHANCED_PWM
	pwmGlowInitialise();
#endif // MCU_HAS_ENHANCED_PWM

#ifdef MCU_HAS_ADVANCED_PWM
	pwmGlowInitialise();
#endif // MCU_HAS_ENHANCED_PWM
	
	// Main loop -------------------------------------------------------
	
	while (1) {
		if (_blinkCount == 0) {
			_blinkCount = LED_BLINK_COUNT;
			
			// Blink that LED
			gpioWrite(&_blinkPin, _blinkState);
			_blinkState = !_blinkState;
		}
		
		if (_glowCount == 0) {
			_glowCount = LED_GLOW_COUNT;
			
			// Glow that other LED
#ifdef MCU_HAS_PCA
			pcaGlowUpdateDutyCycle();
#endif // MCU_HAS_PCA

#ifdef MCU_HAS_ENHANCED_PWM
			pwmGlowUpdateDutyCycle();
#endif // MCU_HAS_ENHANCED_PWM

#ifdef MCU_HAS_ADVANCED_PWM
			pwmGlowUpdateDutyCycle();
#endif // MCU_HAS_ADVANCED_PWM
		}
		
#ifdef ENABLE_PRINTF
		// Echo characters typed on the host
		uint8_t c;
		
		if (uartGetBlock(CONSOLE_UART, &c, 1, NON_BLOCKING)) {
			if (c == '\n') {
				uartSendCharacter(CONSOLE_UART, '\r', BLOCKING);
			}
			
			uartSendCharacter(CONSOLE_UART, c, BLOCKING);
		}
#endif
	}
}
