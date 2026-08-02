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
#include "project-defs.h"
#include <sysclk-hal.h>

#if MCU_FAMILY != 8
	#error "Only the STC8 series are supported by sysclk-hal."
#endif

void clkStartOscillator(ClkOscillator osc) {
	uint8_t v;
	
	switch (osc) {
	case CLK_HIGH_SPEED_IRC:
		HIRCCR = M_ENHIRC;
		while (!(HIRCCR & M_HIRCST));
		break;
	
	case CLK_HIGH_SPEED_XTAL:
		v = M_ENXOSC | M_XITYPE;
#ifdef MCU_HAS_ENHANCED_XOSCCR
		v |= M_GAINX;
	#if MCU_FREQ > 24000000ul
		v |= XCFILTER_MAX_48MHZ << P_XCFILTER;
	#elif MCU_FREQ > 12000000ul
		v |= XCFILTER_MAX_24MHZ << P_XCFILTER;
	#else
		v |= XCFILTER_MAX_12MHZ << P_XCFILTER;
	#endif
#endif
		XOSCCR = v;
		while (!(XOSCCR & M_XOSCST));
		break;
	
#ifdef MCU_HAS_RTC
	case CLK_32KHZ_XTAL:
		X32KCR = M_ENX32K | M_GAIN32K;
		while (!(X32KCR & M_X32KST));
		break;
#endif
	
	case CLK_32KHZ_IRC:
		IRC32KCR = M_ENIRC32K;
		while (!(IRC32KCR & M_IRC32KST));
		break;
	}
}

void clkStopOscillator(ClkOscillator osc) {
	switch (osc) {
	case CLK_HIGH_SPEED_IRC:
		HIRCCR &= ~M_ENHIRC;
		while (HIRCCR & M_HIRCST);
		break;
	
	case CLK_HIGH_SPEED_XTAL:
		XOSCCR &= ~M_ENXOSC;
		while (XOSCCR & M_XOSCST);
		break;
	
#ifdef MCU_HAS_RTC
	case CLK_32KHZ_XTAL:
		X32KCR &= ~M_ENX32K;
		while (X32KCR & M_X32KST);
		break;
#endif
	
	case CLK_32KHZ_IRC:
		IRC32KCR &= ~M_ENIRC32K;
		while (IRC32KCR & M_IRC32KST);
		break;
	}
}

void clkSelectOscillator(ClkOscillator osc) {
	CKSEL = osc;
}
