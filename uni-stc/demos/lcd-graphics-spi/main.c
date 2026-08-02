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
#include <lcd/links/lcd-link-spi.h>
#include <lcd/lcd-device.h>
#include <lcd/lcd-graphics.h>
#include <delay.h>
#include <gpio-hal.h>
#include <spi-hal.h>

#ifdef ENABLE_PRINTF
	#include <uart-hal.h>
	#include <serial-console.h>
#endif // ENABLE_PRINTF

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)
// Include the header file of the controller defined by the LCD_CONTROLLER macro.
#include STRINGIFY(lcd/controllers/LCD_CONTROLLER.h)

static LCDSPILinkConfig lcdLink = {
	// CS# = P1.6
	.csOutput = GPIO_PIN_CONFIG(GPIO_PORT1, GPIO_PIN6, GPIO_BIDIRECTIONAL_MODE),
	// D/C# = P1.7
	.commandDataOutput = GPIO_PIN_CONFIG(GPIO_PORT1, GPIO_PIN7, GPIO_BIDIRECTIONAL_MODE),
	// MOSI = P1.3 | (MISO = P1.4) | SCLK = P1.5 | (SS = P5.4 on STC8H8)
	.spiPinSwitch = SPI_PIN_CONFIG,
	.spiBitOrder = LCD_SPI_ORDER,
	.spiMode = LCD_SPI_MODE,
	.spiClockFrequency = 450000UL, // Must be <= LCD_SPI_MAX_FREQ
	// Note: we use a value below 1MHz here because breadboarding
	// causes severe signal degradation at higher frequencies.
};

// RST# = P1.1
LCD_DEVICE_INTERFACE_WITH_RESET(lcdDevice, &lcdLink, GPIO_PORT1, GPIO_PIN1)

LCD_DEVICE_GRAPHICS(lcdDevice, LCD_TEXT_WIDTH, LCD_TEXT_HEIGHT, LCD_PIXEL_WIDTH, LCD_PIXEL_HEIGHT)

#include "smiley.xbm"

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
	
	lcdInitialiseDevice(&lcdDevice);
	lcdGfxInitialiseDisplayMode(&lcdDevice);
	
	// Main loop -------------------------------------------------------
	
	while (1) {
		lcdGfxClear(&lcdDevice);
		lcdGfxXbmImage(&lcdDevice, smiley_width, smiley_height, smiley_bits, LCD_ALIGN_MIDDLE_CENTER, 0, 0, LCD_ON);
		lcdGfxUpdateDisplay(&lcdDevice);
		delay1ms(1000);
		
		lcdGfxClear(&lcdDevice);
		
		for (uint8_t i = 0; i < lcdDevice.pixelWidth; i += 4) {
			lcdGfxLine(&lcdDevice, i, 0, i, lcdDevice.pixelHeight - 1, LCD_ON);
		}
		
		lcdGfxUpdateDisplay(&lcdDevice);
		delay1ms(1000);
		
		lcdGfxClear(&lcdDevice);
		
		for (uint8_t i = 0; i < lcdDevice.pixelHeight; i += 4) {
			lcdGfxLine(&lcdDevice, 0, i, lcdDevice.pixelWidth - 1, i, LCD_ON);
		}
		
		lcdGfxUpdateDisplay(&lcdDevice);
		delay1ms(1000);
		
		lcdGfxClear(&lcdDevice);
		lcdGfxLine(&lcdDevice, 0, 0, lcdDevice.pixelWidth - 1, lcdDevice.pixelHeight - 1, LCD_ON);
		lcdGfxLine(&lcdDevice, 0, lcdDevice.pixelHeight - 1, lcdDevice.pixelWidth - 1, 0, LCD_ON);
		lcdGfxUpdateDisplay(&lcdDevice);
		delay1ms(1000);
	}
}
