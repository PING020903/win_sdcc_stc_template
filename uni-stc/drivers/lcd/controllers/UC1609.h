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

#ifndef _UC1609_H
#define _UC1609_H

/**
 * **SPI configuration**
 * 
 * Mode 3 (CPOL = 1, CPHA = 1), MSB first, max. 12.5MHz.
 * 
 * **Pin assignments**
 * 
 *  Module pin | MCU pin
 * ------------+---------
 *     K       | GND
 *     A       | +3.3V
 *     GND     | GND
 *     VDD     | +3.3V
 *     SCK     | SCLK
 *     SDA     | MOSI
 *     RST     | RESET#
 *     CD      | D/C#
 *     CS      | CS#
 * 
 * **Note**
 * 
 * The RST pin _MUST_ be connected, power-on reset is not enough.
 */

#define LCD_TEXT_WIDTH 0
#define LCD_TEXT_HEIGHT 0
#define LCD_PIXEL_WIDTH 192
#define LCD_PIXEL_HEIGHT 64

#define LCD_SPI_MAX_FREQ 12500000UL
#define LCD_SPI_MODE SPI_MODE3
#define LCD_SPI_ORDER SPI_MSB_FIRST

#endif // _UC1609_H
