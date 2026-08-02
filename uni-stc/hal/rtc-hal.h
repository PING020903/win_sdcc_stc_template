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
#ifndef _RTC_HAL_H
#define _RTC_HAL_H

/**
 * @file rtc-hal.h
 * 
 * RTC abstraction definitions.
 * 
 * Supported MCU:
 * 
 *     STC8H1KxxT
 *     STC8H2KxxU
 *     STC8H4KxxTL
 *     STC8H4KxxTLCD
 *     STC8H8KxxU
 * 
 * Dependencies:
 * 
 *     sysclk-hal
 * 
 * Macros:
 * 
 *     HAL_RTC_USE_RC_OSCILLATOR uses the 32kHz internal RC oscillator.
 *     The default is to use the external 32kHz crystal oscillator.
 * 
 *     HAL_RTC_ON_ALARM_HANDLER will cause an rtcOnAlarm() function to 
 *     be called when the RTC alarm fires. This function must be defined 
 *     in your code. The default is to implement the RTC ISR yourself.
 */

/**
 * Configures the current date, which will be effective only after
 * calling rtcClockEnable().
 * 
 * @param day The current day (1..31).
 * @param month The current month (1..12).
 * @param year The current year (0..99, assumed to be in the 21st century).
 */
void rtcClockSetDate(uint8_t day, uint8_t month, uint8_t year);
/**
 * Configures the current time, which will be effective only after
 * calling rtcClockEnable().
 * 
 * @param hour The current hour (0..23).
 * @param minute The current minute (0..59).
 * @param second The current second (0..59).
 */
void rtcClockSetTime(uint8_t hour, uint8_t minute, uint8_t second);
/**
 * Configures the RTC and starts it. rtcClockSetDate() and
 * rtcClockSetTime() MUST be called before rtcClockEnable()
 * to correctly initialise the date and time.
 */
void rtcClockEnable();
/**
 * @return the current day.
 */
uint8_t rtcClockGetDay();
/**
 * @return the current month.
 */
uint8_t rtcClockGetMonth();
/**
 * @return the current year (on 2 digits only, assumed to be in the 21st century).
 */
uint8_t rtcClockGetYear();
/**
 * @return the current hour.
 */
uint8_t rtcClockGetHour();
/**
 * @return the current minute.
 */
uint8_t rtcClockGetMinute();
/**
 * @return the current second.
 */
uint8_t rtcClockGetSecond();

/**
 * Configures the time of the alarm.
 */
void rtcAlarmSetTime(uint8_t hour, uint8_t minute, uint8_t second);
/**
 * Enables the alarm interrupt.
 */
void rtcAlarmEnable();
/**
 * Disables the alarm interrupt.
 */
void rtcAlarmDisable();
/**
 * @return true if the alarm interrupt is enabled.
 */
bool rtcAlarmIsEnabled();
/**
 * @return the alarm hour.
 */
uint8_t rtcAlarmGetHour();
/**
 * @return the alarm minute.
 */
uint8_t rtcAlarmGetMinute();
/**
 * @return the alarm second.
 */
uint8_t rtcAlarmGetSecond();

// rtc-hal.h MUST be included in the source file defining main().
INTERRUPT(rtc_isr, RTC_INTERRUPT);

/**
 * When HAL_RTC_ON_ALARM_HANDLER is NOT defined (the default),
 * you must implement the RTC ISR yourself for full access to
 * the RTC's interrupt capabilities. But if you just want to
 * use the alarm interrupt, you can define HAL_RTC_ON_ALARM_HANDLER
 * and the HAL will do it for you.
 */
#ifdef HAL_RTC_ON_ALARM_HANDLER
	/**
	 * This function is invoked when the alarm interrupt is triggered.
	 * You MUST provide its definition.
	 */
	void rtcOnAlarm();
#endif // HAL_RTC_ON_ALARM_HANDLER

#endif // _RTC_HAL_H
