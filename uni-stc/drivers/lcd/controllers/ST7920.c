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

/**
 * @file ST7920.c
 * 
 * Driver for graphics-capable LCD devices using an ST7920 controller.
 * 
 * The ST7920 is found on 12864 LCD devices; these can be used in text 
 * mode (4 lines of 16 latin characters, or 8 Chinese characters), or 
 * in 128x64 pixels graphics mode.
 * 
 * This file implements the abstraction layer described in 
 * lcd-controller.h, which is why it doesn't have its own header file. 
 * This design decision was driven by MCS-51/SDCC restrictions on 
 * function calls made through pointers. As a consequence, if the 
 * application uses several LCD devices, ALL of them **MUST** be based 
 * on the same controller model.
 * 
 */

#include "project-defs.h"
#include <lcd/lcd-controller.h>
#include <delay.h>

#define WRITE_DELAY 72

static void _waitWhileBusy(LCDInterface *interface) {
	while (lcdReadStatus(interface) & 0x80) delay1us(10);
}

static void _sendCommand(LCDDevice *device, uint8_t command) {
	_waitWhileBusy(device->interface);
	lcdSendCommand(device->interface, command);
	device->_status.setAddressInvoked = false;
	// All instructions take 72us except Clear Display
	delay1us(WRITE_DELAY);
}

static void _sendData(LCDInterface *interface, uint8_t data) {
	_waitWhileBusy(interface);
	lcdSendData(interface, data);
	// All data write operations take 72us
	delay1us(WRITE_DELAY);
}

static uint8_t _readData(LCDInterface *interface) {
	_waitWhileBusy(interface);
	uint8_t result = lcdReadData(interface);
	
	return result;
}

static void _functionSet(LCDDevice *device) {
	uint8_t dl = lcdGetLinkWidth(device->interface) == 8 ? 0x10 : 0;
	uint8_t re = device->_status.extendedInstructionSet ? 4 : 0;
	uint8_t g = device->_status.graphicsMode ? 2 : 0;
	_sendCommand(device, 0x20 | dl | re | g);
}

static void _selectExtendedInstructionSet(LCDDevice *device) {
	if (!device->_status.extendedInstructionSet) {
		device->_status.extendedInstructionSet = true;
		_functionSet(device);
	}
}

static void _selectBasicInstructionSet(LCDDevice *device) {
	if (device->_status.extendedInstructionSet) {
		device->_status.extendedInstructionSet = false;
		_functionSet(device);
	}
}

static void _setVerticalScrollMode(LCDDevice *device, bool sr) {
	if (device->_status.verticalScroll != sr) {
		_selectExtendedInstructionSet(device);
		// sr == 1 => enable vertical scroll mode
		// sr == 0 => enable "Set CGRAM Address" instruction (basic instruction set)
		device->_status.verticalScroll = sr;
		_sendCommand(device, 0x02 | sr);
	}
}

void lcdClearTextDisplay(LCDDevice *device) {
	_selectBasicInstructionSet(device);
	_sendCommand(device, 0x01);
	// Clear Display takes 1.6ms but we already waited for WRITE_DELAY in sendCommand()
	delay1us((uint8_t) (1600 - WRITE_DELAY));
}

void lcdSetEntryMode(LCDDevice *device, bool textDirection, bool shiftDisplay) {
	_selectBasicInstructionSet(device);
	// 1 = shift display when cursor goes beyond display edge
	// 0 = don't shift
	uint8_t s = shiftDisplay ? 1 : 0;
	// 1 = increment address counter => move cursor right (LTR text) or shift display left, 
	// 0 = decrement address counter => move cursor left (RTL text) or shift display right
	uint8_t id = textDirection ? 2 : 0;
	_sendCommand(device, 0x04 | id | s);
}

void lcdDisplayControl(LCDDevice *device, bool displayOn, bool cursorOn, bool blinkCursor) {
	_selectBasicInstructionSet(device);
	// Set entire display on/off
	uint8_t d = displayOn ? 4 : 0;
	// Set cursor on/off
	uint8_t c = cursorOn ? 2 : 0;
	// Cursor blink on/off
	uint8_t s = blinkCursor ? 1 : 0;
	_sendCommand(device, 0x08 | d | c | s);
}

void lcdInitialiseController(LCDDevice *device) {
	device->_status.extendedInstructionSet = false;
	device->_status.graphicsMode = false;
	device->_status.verticalScroll = false;
	device->_status.setAddressInvoked = false;
	
    lcdLinkConfigurationBegins(device->interface);
	
	// Data sheet says to wait for > 40ms
	delay1ms(45);
    
	// Issue external reset if needed.
	if (device->interface->resetOutput.count) {
		gpioWrite(&device->interface->resetOutput, 0);
		// Data sheet says 10us
		delay1us(10);
		gpioWrite(&device->interface->resetOutput, 1);
	}

	if (lcdIsLinkParallel(device->interface)) {
		// Configure parallel interface mode (Function Set command)
		if (lcdGetLinkWidth(device->interface) == 8) {
			// 8-bit mode
			_sendCommand(device, 0x30);
			// Data sheet says > 100us and we already waited for WRITE_DELAY
			delay1us(100 - WRITE_DELAY);
			_sendCommand(device, 0x30);
		} else {
			// 4-bit mode
			_sendCommand(device, 0x20);
			// Data sheet says > 100us and we already waited for WRITE_DELAY
			delay1us(100 - WRITE_DELAY);
			_sendCommand(device, 0x20);
			// Data sheet says > 100us and we already waited for WRITE_DELAY
			delay1us(100 - WRITE_DELAY);
		}
	}
    
    lcdLinkConfigurationComplete(device->interface);

	// Turn display on, no cursor, no blinking.
	lcdDisplayControl(device, true, false, false);

	// Clear display
    lcdClearTextDisplay(device);

	// Set Entry Mode (required, even if we want to use the device
	// in graphics mode only).
	lcdSetEntryMode(device, true, false);
}

void lcdWriteByte(LCDDevice *device, uint8_t byte) {
	_sendData(device->interface, byte);
	device->_status.setAddressInvoked = false;
}

uint8_t lcdReadByte(LCDDevice *device) {
	// A "dummy" read MUST be performed immediately after 
	// a setAddress command, before reading data bytes.
	if (device->_status.setAddressInvoked) {
		_readData(device->interface);
		device->_status.setAddressInvoked = false;
	}
	
	return _readData(device->interface);
}

uint8_t lcdReadBusyFlagAndAddress(LCDDevice *device) {
	return lcdReadStatus(device->interface);
}

void lcdReturnHome(LCDDevice *device) {
	_selectBasicInstructionSet(device);
	_sendCommand(device, 0x02);
}

void lcdCursorDisplayShiftControl(LCDDevice *device, bool shiftDisplay, bool shiftRight) {
	_selectBasicInstructionSet(device);
	// shiftDisplay == 1 && shiftRight == 1 => display shifts right, cursor follows, address counter does not change
	// shiftDisplay == 1 && shiftRight == 0 => display shifts left,  cursor follows, address counter does not change
	// shiftDisplay == 0 && shiftRight == 1 => cursor moves right, address counter is incremented
	// shiftDisplay == 0 && shiftRight == 0 => cursor moves left,  address counter is decremented
	// 1 = Shift display, 0 = Move cursor
	uint8_t sc = shiftDisplay ? 8 : 0;
	// 1 = Shift/move to the right, 0 = Shift/move to the left
	uint8_t rl = shiftRight ? 4 : 0;
	_sendCommand(device, 0x10 | sc | rl);
}

void lcdEnableGraphicsDisplay(LCDDevice *device) {
	if (!device->_status.graphicsMode) {
		_selectExtendedInstructionSet(device);
		device->_status.graphicsMode = true;
		_functionSet(device);
	}
}

void lcdDisableGraphicsDisplay(LCDDevice *device) {
	if (device->_status.graphicsMode) {
		_selectExtendedInstructionSet(device);
		device->_status.graphicsMode = false;
		_functionSet(device);
	}
}

void lcdSetCharacterGeneratorAddress(LCDDevice *device, uint8_t address) {
	_setVerticalScrollMode(device, 0); // Required (see datasheet)
	_selectBasicInstructionSet(device);
	_sendCommand(device, 0x40 | (address & 0x3F));
	device->_status.setAddressInvoked = true;
}

void lcdSetTextDisplayAddress(LCDDevice *device, uint8_t address) {
	_selectBasicInstructionSet(device);
	_sendCommand(device, 0x80 | (address & 0x3f));
	device->_status.setAddressInvoked = true;
}

void lcdSetTextDisplayPosition(LCDDevice *device, uint8_t row, uint8_t column) {
	lcdSetTextDisplayAddress(device, ((row & 0x01) << 4) | ((row & 0x02) ? 0x08 : 0x00) | ((column >> 1) & 0x07));
}

void lcdSetGraphicsDisplayAddress(LCDDevice *device, uint8_t pixelX, uint8_t pixelY) {
	_selectExtendedInstructionSet(device);
	uint8_t colAddr = pixelX >> 4;
	uint8_t verticalAddr = pixelY & 0x1f;
	uint8_t horizontalAddr = (colAddr & 0x07) | ((pixelY & 0x20) ? 0x08 : 0);
	_sendCommand(device, 0x80 | verticalAddr);
	_sendCommand(device, 0x80 | horizontalAddr);
	device->_status.setAddressInvoked = true;
}

void lcdEnterStandbyMode(LCDDevice *device) {
	_selectExtendedInstructionSet(device);
	_sendCommand(device, 0x01);
	// Any other subsequent command terminates standby mode,
	// so there's no need for a leaveStandbyMode() function.
}

void lcdLeaveStandbyMode(LCDDevice *device) {
}

void lcdReverseRow(LCDDevice *device, uint8_t row) {
	_selectExtendedInstructionSet(device);
	// Toggles the appearance of the given row between normal and reverse mode.
	_sendCommand(device, 0x04 | (row & 3));
}

void lcdSetScrollAddress(LCDDevice *device, uint8_t address) {
	_selectExtendedInstructionSet(device);
	_sendCommand(device, 0x40 | (address & 0x3f));
}

void lcdEnableVerticalScroll(LCDDevice *device) {
	_setVerticalScrollMode(device, 1);
}

void lcdDisableVerticalScroll(LCDDevice *device) {
	_setVerticalScrollMode(device, 0);
}

// Functionalities below are not available on this controller.
// ---------------------------------------------------------------------

#pragma save
// Suppress warning "unreferenced function argument"
#pragma disable_warning 85

void lcdInverseDisplay(LCDDevice *device, bool on) {
}

void lcdAllPixelsOn(LCDDevice *device, bool on) {
}

#pragma restore
