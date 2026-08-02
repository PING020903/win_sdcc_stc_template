/*
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Copyright (c) 2023 Vincent DEFERT. All rights reserved.
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
#include <lcd/lcd-graphics.h>
#include <lcd/lcd-controller.h>
#include <stdlib.h>
#include <string.h>

/**
 * @file lcd-graphics.c
 * 
 * Graphics operations for LCD devices: implementation.
 */

static void _beginOperation(LCDDevice *device) {
	if (!device->_batchStarted) {
		device->_batchStarted = true;
		device->_minExtentX = device->pixelWidth;
		device->_minExtentY = device->pixelHeight;
		device->_maxExtentX = 0;
		device->_maxExtentY = 0;
	}
}

static void _endOperation(LCDDevice *device) {
	if (device->_autoUpdate) {
		lcdGfxUpdateDisplay(device);
	}
}

void lcdGfxInitialiseDisplayMode(LCDDevice *device) {
	device->_autoUpdate = false;
	device->_batchStarted = false;
	lcdGfxClear(device);
	
	if (device->textWidth) {
		// The ST7920 supports both text and graphics modes
		
		// This is the fastest way to clear the graphics display (< 2 ms)
		lcdDisableGraphicsDisplay(device);
		lcdClearTextDisplay(device);
		
		// Switch to graphics mode
		lcdEnableGraphicsDisplay(device);
		
		device->_batchStarted = false;
	}
}

void lcdGfxEnableAutoUpdate(LCDDevice *device) {
	device->_autoUpdate = true;
}

void lcdGfxDisableAutoUpdate(LCDDevice *device) {
	lcdGfxUpdateDisplay(device);
	device->_autoUpdate = false;
}

void lcdGfxUpdateDisplay(LCDDevice *device) {
	if (device->_batchStarted) {
		device->_batchStarted = false;
		
		if (device->textWidth) {
			// The ST7920 supports both text and graphics modes --------
			
			// The ST7920 requires data bytes be sent in pairs, so we 
			// want to align xMin and xMax on 16-bit boundaries.
			uint8_t xMin = device->_minExtentX & ~0x0f;
			uint8_t xMax = device->_maxExtentX | 0x0f;
			
			for (uint8_t y = device->_minExtentY; y <= device->_maxExtentY; y++) {
				lcdSetGraphicsDisplayAddress(device, xMin, y);
				unsigned int byteOffset = (y * device->_bytesWidth + (xMin / 8));
				
				for (uint8_t x = 0; x <= (xMax - xMin) / 8; x++) {
					lcdWriteByte(device, device->displayBuffer[byteOffset + x]);
				}
			}
		} else {
			// Graphics-only displays (not just ST756x) ----------------
			
			// Align yMin and yMax on byte boundaries.
			uint8_t yMin = device->_minExtentY & ~7;
			uint8_t yMax = device->_maxExtentY | 7;
			
			for (uint8_t y = yMin; y <= yMax; y += 8) {
				lcdSetGraphicsDisplayAddress(device, device->_minExtentX, y);
				
				for (uint8_t x = device->_minExtentX; x <= device->_maxExtentX; x++) {
					uint8_t byte = 0;
					
					for (uint8_t i = 0; i < 8; i++) {
						unsigned int byteOffset = ((y + i) * device->_bytesWidth + (x / 8));
						uint8_t bitMask = 1 << (7 - x % 8);
						byte >>= 1;
						
						if (device->displayBuffer[byteOffset] & bitMask) {
							byte |= 0x80;
						}
					}
					
					lcdWriteByte(device, byte);
				}
			}
		}
	}
}

void lcdGfxClear(LCDDevice *device) {
	_beginOperation(device);
	
	for (unsigned int i = 0; i < (device->pixelHeight * device->_bytesWidth); i++) {
		device->displayBuffer[i] = 0;
	}
	
	device->_minExtentX = 0;
	device->_minExtentY = 0;
	device->_maxExtentX = device->pixelWidth - 1;
	device->_maxExtentY = device->pixelHeight - 1;
	
	_endOperation(device);
}

static void _drawPoint(LCDDevice *device, uint8_t x, uint8_t y, LCDColour colour) {
	unsigned int byteOffset = (y * device->_bytesWidth) + (x / 8);
	
	uint8_t byte = device->displayBuffer[byteOffset];
	uint8_t bitMask = 1 << (7 - x % 8);
	
	// Set the bit according to the requested "colour"
	switch (colour) {
	case LCD_OFF:
		byte &= ~bitMask;
		break;
	case LCD_ON:
		byte |= bitMask;
		break;
	case LCD_INVERSE:
		byte ^= bitMask;
		break;
	}
	
	device->displayBuffer[byteOffset] = byte;
	
	if (x < device->_minExtentX) {
		device->_minExtentX = x;
	}
	
	if (x > device->_maxExtentX) {
		device->_maxExtentX = x;
	}
	
	if (y < device->_minExtentY) {
		device->_minExtentY = y;
	}
	
	if (y > device->_maxExtentY) {
		device->_maxExtentY = y;
	}
}

void lcdGfxPoint(LCDDevice *device, uint8_t x, uint8_t y, LCDColour colour) {
	_beginOperation(device);
	_drawPoint(device, x, y, colour);
	_endOperation(device);
}

void lcdGfxLine(LCDDevice *device, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, LCDColour colour) {
	_beginOperation(device);
	
	if (x0 == x1) {
		if (y0 > y1) {
			uint8_t n = y1;
			y1 = y0;
			y0 = n;
		}
		
		for (uint8_t y = y0; y <= y1; y++) {
			_drawPoint(device, x0, y, colour);
		}
	} else if (y0 == y1) {
		if (x0 > x1) {
			uint8_t n = x1;
			x1 = x0;
			x0 = n;
		}
		
		for (uint8_t x = x0; x <= x1; x++) {
			_drawPoint(device, x, y0, colour);
		}
	} else {
		int dx = abs(x1 - x0);
		int sx = x0 < x1 ? 1 : -1;
		int dy = -abs(y1 - y0);
		int sy = y0 < y1 ? 1 : -1;
		int err = dx + dy;
		int e2;
		int x = x0;
		int y = y0;
		
		while (1) {
			_drawPoint(device, x, y, colour);
			e2 = 2 * err;
			
			if (e2 >= dy) {
				if (x == x1)
					break;
				
				err += dy;
				x += sx;
			}
			
			if (e2 <= dx) {
				if (y == y1)
					break;
				
				err += dx;
				y += sy;
			}
		}
	}
	
	_endOperation(device);
}

void lcdGfxXbmImage(
	LCDDevice *device, 
	uint8_t imageWidth, 
	uint8_t imageHeight, 
	const uint8_t *imageBits, 
	LCDAlignment alignment, 
	uint8_t positionX, 
	uint8_t positionY,
	LCDColour colour
) {
	_beginOperation(device);
	
	// XBM format: if imageWidth is not a multiple of 8, unused bits in the last byte are ignored.
	uint8_t bytesPerImageRow = (imageWidth + 7) / 8;
	uint8_t offsetX = 0;
	uint8_t offsetY = 0;
	
	switch (alignment) {
	case LCD_ALIGN_TOP_LEFT:
	case LCD_ALIGN_BOTTOM_LEFT:
	case LCD_ALIGN_MIDDLE_LEFT:
		offsetX = 0;
		break;
	case LCD_ALIGN_TOP_RIGHT:
	case LCD_ALIGN_BOTTOM_RIGHT:
	case LCD_ALIGN_MIDDLE_RIGHT:
		offsetX = (device->pixelWidth > imageWidth) ? (device->pixelWidth - imageWidth) : 0;
		break;
	case LCD_ALIGN_TOP_CENTER:
	case LCD_ALIGN_BOTTOM_CENTER:
	case LCD_ALIGN_MIDDLE_CENTER:
		offsetX = (device->pixelWidth > imageWidth) ? ((device->pixelWidth - imageWidth) / 2) : 0;
		break;
	case LCD_ALIGN_CUSTOM_OFFSET:
		offsetX = positionX;
		break;
	}
	
	switch (alignment) {
	case LCD_ALIGN_TOP_LEFT:
	case LCD_ALIGN_TOP_RIGHT:
	case LCD_ALIGN_TOP_CENTER:
		offsetY = 0;
		break;
	case LCD_ALIGN_BOTTOM_LEFT:
	case LCD_ALIGN_BOTTOM_RIGHT:
	case LCD_ALIGN_BOTTOM_CENTER:
		offsetY = (device->pixelHeight > imageHeight) ? (device->pixelHeight - imageHeight) : 0;
		break;
	case LCD_ALIGN_MIDDLE_LEFT:
	case LCD_ALIGN_MIDDLE_RIGHT:
	case LCD_ALIGN_MIDDLE_CENTER:
		offsetY = (device->pixelHeight > imageHeight) ? ((device->pixelHeight - imageHeight) / 2) : 0;
		break;
	case LCD_ALIGN_CUSTOM_OFFSET:
		offsetY = positionY;
		break;
	}
	
	for (uint8_t row = 0; row < imageHeight; row++) {
		for (uint8_t col = 0; col < bytesPerImageRow; col++) {
			uint8_t byte = imageBits[row * bytesPerImageRow + col];

			for (uint8_t bit = 0; bit < 8; bit++) {
				// XBM format: the leftmost pixel is represented by the least significant bit.
				if (byte & 1) {
					_drawPoint(device, offsetX + col * 8 + bit, offsetY + row, colour);
				}

				byte >>= 1;
			}
		}
	}
	
	_endOperation(device);
}

void lcdGfxRectangle(LCDDevice *device,
	uint8_t rectWidth, uint8_t rectHeight, 
	uint8_t positionX, uint8_t positionY,
	LCDColour colour
) {
	_beginOperation(device);
	
	for (uint8_t dy = 0; dy < rectHeight; dy++) {
		for (uint8_t dx = 0; dx < rectWidth; dx++) {
			_drawPoint(device, positionX + dx, positionY + dy, colour);
		}
	}

	_endOperation(device);
}

void lcdGfxPrint(LCDDevice *device,
	uint8_t charWidth, uint8_t charHeight, const uint8_t *fontBits, unsigned int fontChars, int fontOffset, 
	uint8_t positionX, uint8_t positionY, LCDColour colour, const char *msg
) {
	_beginOperation(device);
	
	uint8_t bytesPerRow = (charWidth + 7) / 8;
	size_t bytesPerChar = bytesPerRow * charHeight;
	size_t maxPrintableChars = (device->pixelWidth - positionX) / charWidth;
	size_t n = strlen(msg);
	
	if (n > maxPrintableChars) {
		n = maxPrintableChars;
	}
	
	for (size_t i = 0; i < n; i++) {
		unsigned int charIndex = msg[i] + fontOffset;
		
		if (charIndex < fontChars) {
			size_t charOffset = charIndex * bytesPerChar;
			unsigned int offsetX = positionX + (i * charWidth);
			
			for (uint8_t row = 0; row < charHeight; row++) {
				uint8_t offsetY = positionY + row;
				uint8_t charX = 0;
				
				for (uint8_t b = 0; b < bytesPerRow; b++) {
					uint8_t byte = fontBits[charOffset + (row * bytesPerRow) + b];
					
					for (uint8_t p = ((charWidth - charX) > 8) ? 8 : (charWidth - charX); p; p--) {
						// Font format: the leftmost pixel is represented by the most significant bit.
						if (byte & 0x80) {
							_drawPoint(device, offsetX + charX, offsetY, colour);
						}
						
						byte <<= 1;
						charX++;
					}
				}
			}
		}
	}

	_endOperation(device);
}
