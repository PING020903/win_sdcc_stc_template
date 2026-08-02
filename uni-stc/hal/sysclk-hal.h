/*
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Copyright (c) 2024 Vincent DEFERT. All rights reserved.
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
#ifndef _SYSCLK_HAL_H
#define _SYSCLK_HAL_H

/**
 * @file sysclk-hal.h
 * 
 * Clock source control API.
 * 
 * Supported MCU:
 * 
 *     STC8*
 * 
 * Dependencies:
 * 
 *     none
 */

typedef enum {
	CLK_HIGH_SPEED_IRC = 0,
	CLK_HIGH_SPEED_XTAL = 1,
#ifdef MCU_HAS_RTC
	CLK_32KHZ_XTAL = 2,
#endif
	CLK_32KHZ_IRC = 3,
} ClkOscillator;

/**
 * Starts the specified oscillator.
 */
void clkStartOscillator(ClkOscillator osc);

/**
 * Stops the specified oscillator.
 */
void clkStopOscillator(ClkOscillator osc);

/**
 * Use specified oscillator as system clock.
 * The oscillator must be started before invoking this function.
 */
void clkSelectOscillator(ClkOscillator osc);

#endif // _SYSCLK_HAL_H
