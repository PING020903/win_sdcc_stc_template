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

#ifndef _ST756x_H
#define _ST756x_H

/**
 * Supports ST7565R, ST7567, ST7567A and probably more.
 * 
 * **SPI configuration**
 * 
 * Mode 0 (CPOL = 0, CPHA = 0), MSB first, max. 20MHz.
 * 
 * **Pin assignments - GMG12864-06D module (ST7565R)**
 * 
 *  Module pin | MCU pin
 * ------------+---------
 *     CS      | CS#
 *     RSE     | RESET#
 *     RS      | D/C#
 *     SCL     | SCLK
 *     SI      | MOSI
 *     VDD     | +3.3V
 *     VSS     | GND
 *     A       | +3.3V
 *     K       | GND
 *     IC_SCL  | Not used (SPI character generator)
 *     IC_CS   | Not used (SPI character generator)
 *     IC_SO   | Not used (SPI character generator)
 *     IC_SI   | Not used (SPI character generator)
 * 
 * **Note - GMG12864-06D module**
 * 
 * The RST pin _MUST_ be connected, power-on reset is not enough.
 * 
 * **Pin assignments - GM12864-03A module (ST7567)**
 * 
 *  Module pin | MCU pin
 * ------------+---------
 *     CS      | CS#
 *     RST     | RESET#
 *     A0      | D/C#
 *     SCK     | SCLK
 *     SDA     | MOSI
 *     GND     | GND
 *     VDD     | +3.3V
 *     BA      | +3.3V
 *     BK      | GND
 * 
 * **Note - GM12864-03A module**
 * 
 * The RST pin _MUST_ be connected, power-on reset is not enough.
 * 
 * **Pin assignments - GM12864-01A module (ST7567A)**
 * 
 *  Module pin | MCU pin
 * ------------+---------
 *     GND     | GND
 *     VCC     | +3.3V
 *     SCL     | SCLK
 *     SDA     | MOSI
 *     DC      | D/C#
 *     RST     | RESET#
 *     CS      | CS#
 *     BL      | +3.3V
 *     CS-F    | Not used (SPI character generator)
 *     OUT     | Not used (SPI character generator)
 * 
 * **Note - GM12864-01A module**
 * 
 * The RST pin must _NOT_ be connected, power-on reset is enough.
 */

#define LCD_TEXT_WIDTH 0
#define LCD_TEXT_HEIGHT 0
#define LCD_PIXEL_WIDTH 128
#define LCD_PIXEL_HEIGHT 64

#define LCD_SPI_MAX_FREQ 20000000UL
#define LCD_SPI_MODE SPI_MODE0
#define LCD_SPI_ORDER SPI_MSB_FIRST

#endif // _ST756x_H
