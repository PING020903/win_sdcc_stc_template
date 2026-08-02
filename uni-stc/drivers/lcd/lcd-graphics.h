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
#ifndef _LCD_GRAPHICS_H
#define _LCD_GRAPHICS_H

#include <lcd/lcd-device.h>

/**
 * @file lcd-graphics.h
 * 
 * Graphics operations for LCD devices: definitions.
 */

typedef enum {
	LCD_ALIGN_TOP_LEFT,
	LCD_ALIGN_TOP_CENTER,
	LCD_ALIGN_TOP_RIGHT,
	LCD_ALIGN_BOTTOM_LEFT,
	LCD_ALIGN_BOTTOM_CENTER,
	LCD_ALIGN_BOTTOM_RIGHT,
	LCD_ALIGN_MIDDLE_LEFT,
	LCD_ALIGN_MIDDLE_RIGHT,
	LCD_ALIGN_MIDDLE_CENTER,
	LCD_ALIGN_CUSTOM_OFFSET,
} LCDAlignment;

typedef enum {
	LCD_OFF,
	LCD_ON,
	LCD_INVERSE,
} LCDColour;

void lcdGfxInitialiseDisplayMode(LCDDevice *device);

void lcdGfxEnableAutoUpdate(LCDDevice *device);

void lcdGfxDisableAutoUpdate(LCDDevice *device);

void lcdGfxUpdateDisplay(LCDDevice *device);

void lcdGfxClear(LCDDevice *device);

void lcdGfxPoint(LCDDevice *device, uint8_t x, uint8_t y, LCDColour colour);

void lcdGfxLine(LCDDevice *device, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, LCDColour colour);

/*
 * Usage:

	#include "smiley.xbm"

	// positionX and positionY are ignored for all alignments except LCD_ALIGN_CUSTOM_OFFSET
	lcdGfxXbmImage(gfxInfo, smiley_width, smiley_height, smiley_bits, LCD_ALIGN_MIDDLE_CENTER, 0, 0, LCD_ON);
*/
void lcdGfxXbmImage(
	LCDDevice *device, 
	uint8_t imageWidth, 
	uint8_t imageHeight, 
	const uint8_t *imageBits, 
	LCDAlignment alignment, 
	uint8_t positionX, 
	uint8_t positionY,
	LCDColour colour
);

/**
 * Draws a rectangle.
 *
 * @param device Pointer to the display module's configuration and state.
 * @param rectWidth Width of the rectangle.
 * @param rectHeight Height of the rectangle.
 * @param positionX Position of the top left corner of the rectangle.
 * @param positionY Position of the top left corner of the rectangle.
 * @param colour Colour of the rectangle.
 */
void lcdGfxRectangle(LCDDevice *device,
	uint8_t rectWidth, uint8_t rectHeight, 
	uint8_t positionX, uint8_t positionY,
	LCDColour colour
);

/**
 * Prints a string using a fixed-width bitmap font.
 * 
 * The bits of the font that are 0 are transparent, the others are 
 * drawn using the specified colour.
 *
 * @param device Pointer to the display module's configuration and state.
 * @param charWidth Width of a character in pixels.
 * @param charHeight Height of a character in pixels.
 * @param fontBits Bitmap font.
 * @param fontChars Number of characters encoded in the font.
 * @param fontOffset Value added to the code of a character to obtain its index in the font.
 * @param positionX Position of the top left corner of the text.
 * @param positionY Position of the top left corner of the text.
 * @param colour Colour of the text.
 * @param msg Text to print.
 */
void lcdGfxPrint(LCDDevice *device,
	uint8_t charWidth, uint8_t charHeight, const uint8_t *fontBits, unsigned int fontChars, int fontOffset, 
	uint8_t positionX, uint8_t positionY, LCDColour colour, const char *msg
);

#endif // _LCD_GRAPHICS_H
