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

#ifdef ENABLE_PRINTF
	#include <uart-hal.h>
	#include <serial-console.h>
#endif // ENABLE_PRINTF

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
	
	while (1) {
		printf("CHIP_GUID:");
		
		for (uint8_t i = 0; i < elementsof(CHIP_GUID); i++) {
			printf(" %02hhX", CHIP_GUID[i]);
		}
		
		printf("\n");
		
		printf("CHIP_PART_ID: %04X\n", CHIP_PART_ID);
		printf("INTERNAL_VOLTAGE_REF: %u mV\n", INTERNAL_VOLTAGE_REF_mV);
		printf("WKT_FREQ: %u Hz\n", WKT_FREQ_Hz);
		printf("IRTRIM_22_1184MHz: %0hhX\n", IRTRIM_22_1184MHz);
		printf("IRTRIM_24MHz: %0hhX\n", IRTRIM_24MHz);
		
#if defined(CHIPID_IN_XDATA)
		printf("Program/EEPROM split address: %04X\n", PROGRAM_AREA_SIZE);
#endif
	}
}
