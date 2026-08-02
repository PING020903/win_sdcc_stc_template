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

#ifndef MCU_HAS_RTC
	#error "The target MCU doesn't have an RTC."
#endif

#include <rtc-hal.h>
#include <sysclk-hal.h>

void rtcClockSetDate(uint8_t day, uint8_t month, uint8_t year) {
	INIDAY = day;
	INIMONTH = month;
	INIYEAR = year;
}

void rtcClockSetTime(uint8_t hour, uint8_t minute, uint8_t second) {
	INIHOUR = hour;
	INIMIN = minute;
	INISEC = second;
	INISSEC = 0;
}

void rtcClockEnable() {
#ifdef HAL_RTC_USE_RC_OSCILLATOR
	// NOTE: the INTERNAL 32kHz RC oscillator is not precise
	// enough to be used for anything except quick testing.
	
	// Start internal 32kHz RC oscillator.
	clkStartOscillator(CLK_32KHZ_IRC);
	// Select internal 32kHz RC oscillator as RTC clock source.
	RTCCFG |= M_RTCCKS;
#else
	// Start external 32kHz crystal oscillator.
	clkStartOscillator(CLK_32KHZ_XTAL);
	// Select external 32kHz oscillator as RTC clock source.
	RTCCFG &= ~M_RTCCKS;
#endif
	
	// Copy INI* registers into * registers (e.g. INIHOUR to HOUR).
	// This implies rtcClockSetDate() and rtcClockSetTime()
	// have been called before invoking rtcClockEnable();
	RTCCFG |= M_RTCSET;
	
	// Start RTC
	RTCCR |= M_RTCRUN;
}

uint8_t rtcClockGetDay() {
	return RTCDAY;
}

uint8_t rtcClockGetMonth() {
	return RTCMONTH;
}

uint8_t rtcClockGetYear() {
	return RTCYEAR;
}

uint8_t rtcClockGetHour() {
	return RTCHOUR;
}

uint8_t rtcClockGetMinute() {
	return RTCMIN;
}

uint8_t rtcClockGetSecond() {
	return RTCSEC;
}

void rtcAlarmSetTime(uint8_t hour, uint8_t minute, uint8_t second) {
	ALAHOUR = hour;
	ALAMIN = minute;
	ALASEC = second;
	ALASSEC = 0;
}

void rtcAlarmEnable() {
	RTCIEN |= M_ALAIE;
}

void rtcAlarmDisable() {
	RTCIEN &= ~M_ALAIE;
}

bool rtcAlarmIsEnabled() {
	return RTCIEN & M_ALAIE;
}

uint8_t rtcAlarmGetHour() {
	return ALAHOUR;
}

uint8_t rtcAlarmGetMinute() {
	return ALAMIN;
}

uint8_t rtcAlarmGetSecond() {
	return ALASEC;
}

#ifdef HAL_RTC_ON_ALARM_HANDLER

INTERRUPT(rtc_isr, RTC_INTERRUPT) {
	if (RTCIF & M_ALAIF) {
		RTCIF &= ~M_ALAIF;
		rtcOnAlarm();
	}
}

#endif // HAL_RTC_ON_ALARM_HANDLER
