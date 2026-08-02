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

#ifndef _ST7920_H
#define _ST7920_H

/**
 * **Pin assignments - PARALLEL interface**
 * 
 * Signal	Description
 * ---------------------------------------------------------------------
 * VSS		Connect to GND
 * VDD		Connect to +5V
 * VO		Contrast adjustment. Connect to +5V for maximum contrast.
 * RS		Register Select. 0 = Instruction, 1 = Data.
 * RW		Transfer direction. 0 = Write, 1 = Read.
 * E		I/O enable. Enables data out when high. Latches data in on falling edge.
 * D0		Data bus (bidirectional).
 * D1
 * D2
 * D3
 * D4
 * D5
 * D6
 * D7
 * BLA		Backlight anode. Connect to +5V.
 * BLK		Backlight cathode. Connect to GND.
 * PSB		Parallel Serial Bit. 0 = Serial, 1 = Parallel. Connect to +5V.
 * 
 * **Pin assignments - SERIAL interface**
 * Maximum clock frequency: 2.5MHz
 * 
 * Signal	Description
 * ---------------------------------------------------------------------
 * VSS		Connect to GND
 * VDD		Connect to +5V
 * VO		Contrast adjustment. Connect to +5V for maximum contrast.
 * RS		Chip Select (CS)
 * RW		Serial Data (SID)
 * E		Serial Clock (SCLK)
 * BLA		Backlight anode. Connect to +5V.
 * BLK		Backlight cathode. Connect to GND.
 * PSB		Parallel Serial Bit. 0 = Serial, 1 = Parallel. Connect to GND.
 * 
 * **Notes**
 * 
 * - **IMPORTANT**: read operations are not supported in serial mode.
 * This has implications if your application needs to read the display's
 * DDRAM/CGRAM/GDRAM (you need to manage a display buffer, so your MCU 
 * must have enough RAM for this).
 * 
 * - Don't connect the RST pin of the LCD module, it doesn't behave
 * as expected.
 * 
 * - If you find backlight too bright, you may connect BLA to +5V 
 * through a resistor of, say 150 or 180 Ohm.
 * 
 * - If contrast is too weak with VO connected to +5V (text is only 
 * visible when viewed under a small angle), check your power supply 
 * voltage (must be greater than 4.5V).
 * 
 * - In 4-bit parallel mode, only D7..D4 are used, and most significant 
 * nibble is always transfered first.
 * 
 * - In serial mode, the CS signal of the ST7920 is active HIGH.
 * 
 * - If the ST7920 is the only SPI peripheral, CS can be permanently pulled up.
 */

#define LCD_TEXT_WIDTH 16
#define LCD_TEXT_HEIGHT 4
#define LCD_PIXEL_WIDTH 128
#define LCD_PIXEL_HEIGHT 64

// 2.5MHz @5V, 1.666MHz@2.7V
#define LCD_SPI_MAX_FREQ 1666666UL
// Clock idle low, sample on trailing edge
#define LCD_SPI_MODE SPI_MODE1
#define LCD_SPI_ORDER SPI_MSB_FIRST

#endif // _ST7920_H
