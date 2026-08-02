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
#include <spi-hal.h>
#include <gpio-hal.h>
#include <string.h>

/**
 * @file spi-hal.c
 * 
 * SPI abstraction implementation.
 */

#define SPI_PIN_SWITCH 0
#define SPI_SS_PIN 1
#define SPI_MOSI_PIN 2
#define SPI_MISO_PIN 3
#define SPI_SCLK_PIN 4
#define SPI_ROW_SIZE 5

static const uint8_t _pinConfigurations[][SPI_ROW_SIZE] = {
#if MCU_FAMILY == 8
	#if MCU_PINS == 8
		{ 0, 0x55, 0x54, 0x33, 0x32 },
	#elif !(defined(GPIO_NO_P13) || defined(GPIO_NO_P14) || defined(GPIO_NO_P15))
		#if MCU_SERIES == 'H' && defined(GPIO_NO_P12)
			{ 0, 0x54, 0x13, 0x14, 0x15 },
		#elif MCU_PINS >= 20
			{ 0, 0x12, 0x13, 0x14, 0x15 },
		#endif // MCU_SERIES == 'H' && defined(GPIO_NO_P12)
	#endif // MCU_PINS == 8
	
	#if MCU_PINS >= 28
		{ 1, 0x22, 0x23, 0x24, 0x25 },
	#endif // MCU_PINS >= 28
	
	#if (MCU_SERIES == 'G' || MCU_SERIES == 'H') && MCU_PINS >= 44
		{ 2, 0x54, 0x40, 0x41, 0x43 },
	#endif // (MCU_SERIES == 'G' || MCU_SERIES == 'H') && MCU_PINS >= 44
	
	#if MCU_SERIES == 'A' && MCU_PINS == 64
		{ 2, 0x74, 0x75, 0x76, 0x77 },
	#endif // MCU_SERIES == 'A' && MCU_PINS == 64
	
	#if MCU_PINS > 8
		{ 3, 0x35, 0x34, 0x33, 0x32 },
	#endif // MCU_PINS > 8
#endif // MCU_FAMILY == 8

#if MCU_FAMILY == 15
	{ 0, 0x12, 0x13, 0x14, 0x15 },
	
	#if MCU_PINS >= 28
		{ 1, 0x24, 0x23, 0x22, 0x21 },
	#endif // MCU_PINS >= 28
	
	#if MCU_PINS >= 44
		{ 2, 0x54, 0x40, 0x41, 0x43 },
	#endif // MCU_PINS >= 44
#endif // MCU_FAMILY == 15

#if MCU_FAMILY == 12
	{ 0, 0x14, 0x15, 0x16, 0x17 },
	
	#if MCU_PINS >= 44
		{ 1, 0x40, 0x41, 0x42, 0x43 },
	#endif // MCU_PINS >= 44
#endif // MCU_FAMILY == 12
};

// On all STC8G, STC8H and the STC8A8K64D4, GPIO ports are configured
// in high-impedance mode by default, so configuring the output pins
// mode is *REQUIRED*.
static void _configurePins(uint8_t pinSwitch, GpioPinMode outputPinMode) {
	for (uint8_t i = 0; i < (sizeof(_pinConfigurations) / SPI_ROW_SIZE); i++) {
		if (_pinConfigurations[i][SPI_PIN_SWITCH] == pinSwitch) {
			P_SW1 = (P_SW1 & ~M_SPI_S) | ((pinSwitch << P_SPI_S) & M_SPI_S);
			// .port and .pin don't matter, they'll be overriden later.
			GpioConfig pinConfig = GPIO_PIN_CONFIG(GPIO_PORT3, GPIO_PIN0, GPIO_HIGH_IMPEDANCE_MODE);
			uint8_t pinDefinition;
			
#ifdef HAL_SPI_MASTER_MODE
			// Configure input
			pinDefinition = _pinConfigurations[i][SPI_MISO_PIN];
			pinConfig.port = (GpioPort) (pinDefinition >> 4);
			pinConfig.pin = (GpioPin) (pinDefinition & 0x0f);
			gpioConfigure(&pinConfig);
			
			// Configure outputs
			pinConfig.pinMode = outputPinMode;
#ifdef GPIO_HAS_SR_DR_IE
			pinConfig.speed = GPIO_SPEED_FASTEST;
#endif
			pinDefinition = _pinConfigurations[i][SPI_MOSI_PIN];
			pinConfig.port = (GpioPort) (pinDefinition >> 4);
			pinConfig.pin = (GpioPin) (pinDefinition & 0x0f);
			gpioConfigure(&pinConfig);
			
			pinDefinition = _pinConfigurations[i][SPI_SCLK_PIN];
			pinConfig.port = (GpioPort) (pinDefinition >> 4);
			pinConfig.pin = (GpioPin) (pinDefinition & 0x0f);
			gpioConfigure(&pinConfig);
#else // => HAL_SPI_SLAVE_MODE
			// Configure inputs
			pinDefinition = _pinConfigurations[i][SPI_MOSI_PIN];
			pinConfig.port = (GpioPort) (pinDefinition >> 4);
			pinConfig.pin = (GpioPin) (pinDefinition & 0x0f);
			gpioConfigure(&pinConfig);
			
			pinDefinition = _pinConfigurations[i][SPI_SCLK_PIN];
			pinConfig.port = (GpioPort) (pinDefinition >> 4);
			pinConfig.pin = (GpioPin) (pinDefinition & 0x0f);
			gpioConfigure(&pinConfig);
			
			pinDefinition = _pinConfigurations[i][SPI_SS_PIN];
			pinConfig.port = (GpioPort) (pinDefinition >> 4);
			pinConfig.pin = (GpioPin) (pinDefinition & 0x0f);
			gpioConfigure(&pinConfig);
			
			// Configure output
			pinConfig.pinMode = outputPinMode;
#ifdef GPIO_HAS_SR_DR_IE
			pinConfig.speed = GPIO_SPEED_FASTEST;
#endif
			pinDefinition = _pinConfigurations[i][SPI_MISO_PIN];
			pinConfig.port = (GpioPort) (pinDefinition >> 4);
			pinConfig.pin = (GpioPin) (pinDefinition & 0x0f);
			gpioConfigure(&pinConfig);
#endif // HAL_SPI_MASTER_MODE
			break;
		}
	}
}

static HAL_SPI_SEGMENT struct {
	uint8_t *buffer;
	size_t index;
	size_t bufferSize;
	bool *readyFlag;
} _spiState;

SpiSpeed spiSelectSpeed(uint32_t maxDeviceRate) {
	uint16_t divisor = (uint16_t) ((MCU_FREQ + maxDeviceRate - 1) / maxDeviceRate);
	uint8_t pot = 0;
	
	// Determine the power of two ("pot") equal or immediately greater
	// to the value of the divisor.
	for (uint16_t n = divisor; n > 1; n = n >> 1, pot++);
	
	if (divisor > (1 << pot)) {
		pot++;
	}
	
	// Determine SPI clock equal or immediately greater to "pot".
	// SPI_SYSCLK_DIV_4 exists for all MCU, so let's use it as default value.
	SpiSpeed result = SPI_SYSCLK_DIV_4;
	
#if MCU_FAMILY == 12 || MCU_FAMILY == 15
	if (pot > 2 && pot <= 4) {
		result = SPI_SYSCLK_DIV_16;
	} else if (pot > 4 && pot <= 6) {
		result = SPI_SYSCLK_DIV_64;
	} else if (pot > 6) {
		result = SPI_SYSCLK_DIV_128;
	}
#endif // MCU_FAMILY == 12 || MCU_FAMILY == 15
	
#if MCU_FAMILY == 8
	#ifdef SPI_HAS_HIGH_SPEED
		// STC8H3 (B version), STC8H8 (B version), STC8H4
		if (pot > 2 && pot <= 3) {
			result = SPI_SYSCLK_DIV_8;
		} else if (pot < 2) {
			result = SPI_SYSCLK_DIV_2;
		} else if (pot > 3) {
			result = SPI_SYSCLK_DIV_16;
		}
	#else
		// All other STC8
		if (pot > 2 && pot <= 3) {
			result = SPI_SYSCLK_DIV_8;
		} else if (pot == 4) {
			result = SPI_SYSCLK_DIV_16;
		} else if (pot > 4) {
			result = SPI_SYSCLK_DIV_32;
		}
	#endif // SPI_HAS_HIGH_SPEED
#endif // MCU_FAMILY == 8
	
	return result;
}

void spiConfigure(SpiBitOrder bitOrder, SpiMode mode, SpiSpeed speed, uint8_t pinSwitch, GpioPinMode outputPinMode) {
	_configurePins(pinSwitch, outputPinMode);
	SPCTL = bitOrder | mode | speed |
#ifdef HAL_SPI_MASTER_MODE
		M_SSIG | M_MSTR |
#endif
		M_ENSPI;

	IE2 = IE2 | M_SPIEN;
}

void spiSend(uint8_t *buffer, size_t bufferSize, bool *readyFlag) {
	_spiState.buffer = buffer;
	_spiState.bufferSize = bufferSize;
	_spiState.readyFlag = readyFlag;
	*_spiState.readyFlag = false;
	_spiState.index = 0;
	
#ifdef HAL_SPI_MASTER_MODE
	SPDAT = _spiState.buffer[_spiState.index];
#endif
}

void spiReceive(uint8_t *buffer, size_t bufferSize, bool *readyFlag) {
	memset(buffer, 0, bufferSize);
	spiSend(buffer, bufferSize, readyFlag);
}

INTERRUPT(spi_isr, SPI_INTERRUPT) {
	SPSTAT |= M_SPIIF | M_WCOL;
#ifdef HAL_SPI_MASTER_MODE
	// Reminder: the first byte of the buffer was sent by spiSend()
	
	// Replace master's data with slave's in buffer
	_spiState.buffer[_spiState.index] = SPDAT;
	_spiState.index++;
	
	if (_spiState.index < _spiState.bufferSize) {
		// Send next byte to slave
		SPDAT = _spiState.buffer[_spiState.index];
	} else {
		// We're done
		*_spiState.readyFlag = true;
	}
#else // => HAL_SPI_SLAVE_MODE
	// Take data from master
	uint8_t data = SPDAT;
	
	if (_spiState.index < _spiState.bufferSize) {
		// Reply with slave's
		SPDAT = _spiState.buffer[_spiState.index];
		// Replace slave's data with master's in buffer
		_spiState.buffer[_spiState.index] = data;
		_spiState.index++;
		
		if (_spiState.index == _spiState.bufferSize) {
			*_spiState.readyFlag = true;
		}
	} else {
		// Ignore potential data in excess and send zeroes as filler
		SPDAT = 0;
	}
#endif
}
