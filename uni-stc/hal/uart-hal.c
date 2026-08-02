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
#include <uart-hal.h>

#ifndef HAL_UART_DEFAULT_TX_BUFFER_SIZE
	#define HAL_UART_DEFAULT_TX_BUFFER_SIZE 16
#endif

#ifndef HAL_UART_DEFAULT_RX_BUFFER_SIZE
	#define HAL_UART_DEFAULT_RX_BUFFER_SIZE 4
#endif

#ifndef HAL_UART_DEFAULT_SEGMENT
	// Default to the memory model's segment.
	#define HAL_UART_DEFAULT_SEGMENT
#endif

/**
 * @file uart-hal.c
 * 
 * UART abstraction layer implementation for STC12, STC15 and STC8.
 * 
 * TODO: 9-bit modes (parity & multi-machine) support.
 */

#define STATUS_CLEAR   0
#define STATUS_SENDING 1

#ifndef HAL_UART1_TX_BUFFER_SIZE
	#define HAL_UART1_TX_BUFFER_SIZE HAL_UART_DEFAULT_TX_BUFFER_SIZE
#endif

#ifndef HAL_UART1_RX_BUFFER_SIZE
	#define HAL_UART1_RX_BUFFER_SIZE HAL_UART_DEFAULT_RX_BUFFER_SIZE
#endif

#ifndef HAL_UART1_SEGMENT
	#define HAL_UART1_SEGMENT HAL_UART_DEFAULT_SEGMENT
#endif

FIFO_BUFFER(_UART1_receiveBuffer, HAL_UART1_RX_BUFFER_SIZE, HAL_UART1_SEGMENT)
FIFO_BUFFER(_UART1_transmitBuffer, HAL_UART1_TX_BUFFER_SIZE, HAL_UART1_SEGMENT)

#if HAL_UARTS >= 2
	#ifndef HAL_UART2_TX_BUFFER_SIZE
		#define HAL_UART2_TX_BUFFER_SIZE HAL_UART_DEFAULT_TX_BUFFER_SIZE
	#endif

	#ifndef HAL_UART2_RX_BUFFER_SIZE
		#define HAL_UART2_RX_BUFFER_SIZE HAL_UART_DEFAULT_RX_BUFFER_SIZE
	#endif

	#ifndef HAL_UART2_SEGMENT
		#define HAL_UART2_SEGMENT HAL_UART_DEFAULT_SEGMENT
	#endif

	FIFO_BUFFER(_UART2_receiveBuffer, HAL_UART2_RX_BUFFER_SIZE, HAL_UART2_SEGMENT)
	FIFO_BUFFER(_UART2_transmitBuffer, HAL_UART2_TX_BUFFER_SIZE, HAL_UART2_SEGMENT)
#endif // HAL_UARTS >= 2

#if HAL_UARTS >= 3
	#ifndef HAL_UART3_TX_BUFFER_SIZE
		#define HAL_UART3_TX_BUFFER_SIZE HAL_UART_DEFAULT_TX_BUFFER_SIZE
	#endif

	#ifndef HAL_UART3_RX_BUFFER_SIZE
		#define HAL_UART3_RX_BUFFER_SIZE HAL_UART_DEFAULT_RX_BUFFER_SIZE
	#endif

	#ifndef HAL_UART3_SEGMENT
		#define HAL_UART3_SEGMENT HAL_UART_DEFAULT_SEGMENT
	#endif

	FIFO_BUFFER(_UART3_receiveBuffer, HAL_UART3_RX_BUFFER_SIZE, HAL_UART3_SEGMENT)
	FIFO_BUFFER(_UART3_transmitBuffer, HAL_UART3_TX_BUFFER_SIZE, HAL_UART3_SEGMENT)

	#ifndef HAL_UART4_TX_BUFFER_SIZE
		#define HAL_UART4_TX_BUFFER_SIZE HAL_UART_DEFAULT_TX_BUFFER_SIZE
	#endif

	#ifndef HAL_UART4_RX_BUFFER_SIZE
		#define HAL_UART4_RX_BUFFER_SIZE HAL_UART_DEFAULT_RX_BUFFER_SIZE
	#endif

	#ifndef HAL_UART4_SEGMENT
		#define HAL_UART4_SEGMENT HAL_UART_DEFAULT_SEGMENT
	#endif

	FIFO_BUFFER(_UART4_receiveBuffer, HAL_UART4_RX_BUFFER_SIZE, HAL_UART4_SEGMENT)
	FIFO_BUFFER(_UART4_transmitBuffer, HAL_UART4_TX_BUFFER_SIZE, HAL_UART4_SEGMENT)
#endif // HAL_UARTS >= 3

static FifoState *_uartReceiveBuffer(Uart uart) {
	FifoState *result = NULL;
	
	switch (uart) {
	case UART1:
		result = &_UART1_receiveBuffer;
		break;
	
#if HAL_UARTS >= 2
	case UART2:
		result = &_UART2_receiveBuffer;
		break;
#endif // HAL_UARTS >= 2

#if HAL_UARTS >= 3
	case UART3:
		result = &_UART3_receiveBuffer;
		break;
	
	case UART4:
		result = &_UART4_receiveBuffer;
		break;
#endif // HAL_UARTS >= 3
	}
	
	return result;
}

static FifoState *_uartTransmitBuffer(Uart uart) {
	FifoState *result = NULL;
	
	switch (uart) {
	case UART1:
		result = &_UART1_transmitBuffer;
		break;
	
#if HAL_UARTS >= 2
	case UART2:
		result = &_UART2_transmitBuffer;
		break;
#endif // HAL_UARTS >= 2

#if HAL_UARTS >= 3
	case UART3:
		result = &_UART3_transmitBuffer;
		break;
	
	case UART4:
		result = &_UART4_transmitBuffer;
		break;
#endif // HAL_UARTS >= 3
	}
	
	return result;
}

bool uartIsTransmissionComplete(Uart uart) {
	FifoState *buffer = _uartTransmitBuffer(uart);
	
	return buffer->status == STATUS_CLEAR;
}

bool uartTransmitBufferHasBytesFree(Uart uart, uint8_t bytes) {
	EA = 0;
	bool rc = fifoBytesFree(_uartTransmitBuffer(uart)) >= bytes;
	EA = 1;
	
	return rc;
}

uint8_t uartReceiveBufferBytes(Uart uart) {
	EA = 0;
	uint8_t rc = fifoBytesUsed(_uartReceiveBuffer(uart));
	EA = 1;
	
	return rc;
}

void uartFlushReceiveBuffer(Uart uart) {
	EA = 0;
	fifoClear(_uartReceiveBuffer(uart));
	EA = 1;
}

#if !defined(M_S1_S) || !defined(TIMER_HAS_T1) || !defined(TIMER_HAS_T2)
	/*
	 * Suppress warning "unreferenced function argument" when:
	 * 
	 *   - using STC12 with HAL_UARTS set to 1 (M_S1_S not defined),
	 * 
	 *   - selection of the UART's corresponding timer is not possible
	 *     (TIMER_HAS_T1 and/or TIMER_HAS_T2 not defined).
	 */
	#pragma save
	#pragma disable_warning 85
	#define WARNING_85
#endif // M_S1_S
TimerStatus uartInitialise(Uart uart, uint32_t baudRate, UartBaudRateTimer baudRateTimer, UartMode mode, uint8_t pinSwitch) {
	TimerStatus rc = TIMER_FREQUENCY_OK;
#ifdef TIMER_HAS_T2
	Timer timer = TIMER2;
#else
	// Only the 8-pin STC8G1K* have T1 but not T2.
	Timer timer = TIMER1;
#endif // TIMER_HAS_T2

	// UART1 is somewhat peculiar: when mode != UART_8N1, baudRate is
	// expected to be of type Uart1_9BitMode_Clock and is derived from
	// sysclk instead of a timer.
	if (uart != UART1 || mode == UART_8N1) {
// All the #ifdef with TIMER_HAS_T1 and TIMER_HAS_T2 are intended to
// prevent generating code the chosen MCU doesn't support, so as to
// save the developer headaches that can be avoided by design.
#ifdef TIMER_HAS_T2
	#ifdef TIMER_HAS_T1
		if (baudRateTimer == UART_USE_OWN_TIMER) {
			// There's a reason why UART numbers start at 1
			// while timer numbers start at 0!  :)
			timer =  (Timer) uart;
		}
	// #else: If the target MCU doesn't have timer 1, the only possible
	// baud rate generator is timer 2, which is the default.
	#endif // TIMER_HAS_T1
// #else: If the target MCU doesn't have timer 2, it has timer 1, which
// is the default and the only possible choice.
#endif // TIMER_HAS_T2
		
		// Note: on the STC12, TIMER2 is the BRT timer.
		// The timer HAL makes this transparent for the developer.
		rc = startTimer(
			timer, 
			baudRateToSysclkDivisor(baudRate), 
			DISABLE_OUTPUT, 
			DISABLE_INTERRUPT, 
			FREE_RUNNING
		);
	}
	
	if (rc == TIMER_FREQUENCY_OK) {
		uint8_t operationMode = 0;
		
		switch (mode) {
		case UART_8N1:
			if (uart == UART1) {
				operationMode = 1;
			}
			break;
		
		case UART_8E1:
		case UART_8O1:
		case UART_MULTI_MACHINE:
			if (uart == UART1) {
				operationMode = 2 | ((baudRate & 2) >> 1);
			} else {
				operationMode = 1;
			}
			break;
		}
		
		uint8_t scon = ((mode == UART_MULTI_MACHINE) ? M_SM2 : 0)
			| ((operationMode & 2) ? M_SM0 : 0) | M_REN;
		
		switch (uart) {
		case UART1:
			// Configure UART clock source
			switch (operationMode) {
			case 1:
#if defined(M_S1ST2) && defined(TIMER_HAS_T1) && defined(TIMER_HAS_T2)
				if (timer == TIMER2) {
					AUXR |= M_S1ST2;
				} else {
					AUXR &= ~M_S1ST2;
				}
#endif // M_S1ST2
				break;
			
			case 2:
				// Remember baudRate is of type Uart1_9BitMode_Clock.
				if (baudRate & 1) {
					PCON |= M_SMOD;
				} else {
					PCON &= ~M_SMOD;
				}
				break;
			
	#if MCU_FAMILY != 90
			case 3:
				// Remember baudRate is of type Uart1_9BitMode_Clock.
				if (baudRate & 1) {
					AUXR |= M_UART_M0x6;
				} else {
					AUXR &= ~M_UART_M0x6;
				}
				break;
	#endif
			}
			
#ifdef M_S1_S
			// Set pin configuration
			P_SW1 = (P_SW1 & ~M_S1_S) | ((pinSwitch << P_S1_S) & M_S1_S);
#endif // M_S1_S
			
			// Set UART mode and clear interrupt flag
			S1CON = scon | ((operationMode & 1) ? M_SM1 : 0);
			
			// Enable serial port interrupt
			IE1 |= M_S1IE;
			break;
		
#if HAL_UARTS >= 2
		case UART2:
			// For UART2, OWM_TIMER == TIMER2, so no need to 
			// configure UART clock source.
			
	#if MCU_FAMILY == 8 || MCU_FAMILY == 15
			// Set pin configuration
			P_SW2 = (P_SW2 & ~M_S2_S) | ((pinSwitch << P_S2_S) & M_S2_S);
			
			// Set UART mode and clear interrupt flag
			S2CON = scon;
	#endif // MCU_FAMILY == 8 || MCU_FAMILY == 15

	#if MCU_FAMILY == 12
			// Set pin configuration
			P_SW1 = (P_SW1 & ~M_S2_S) | ((pinSwitch << P_S2_S) & M_S2_S);
			
			S2CON = scon | M_SM1; // Yes, that's not a mistake, see TRM.
	#endif // MCU_FAMILY == 12
			
			// Enable serial port interrupt
			IE2 |= M_S2IE;
			break;
#endif // HAL_UARTS >= 2

#if HAL_UARTS >= 3
		case UART3:
			// Configure UART clock source
			if (baudRateTimer == UART_USE_OWN_TIMER) {
				S3CON |= M_S3ST3;
			} else {
				S3CON &= ~M_S3ST3;
			}
			
			// Set pin configuration
			P_SW2 = (P_SW2 & ~M_S3_S) | ((pinSwitch << P_S3_S) & M_S3_S);
			
			// Set UART mode and clear interrupt flag
			S3CON = scon;
			
			// Enable serial port interrupt
			IE2 |= M_S3IE;
			break;

		case UART4:
			// Configure UART clock source
			if (baudRateTimer == UART_USE_OWN_TIMER) {
				S4CON |= M_S4ST4;
			} else {
				S4CON &= ~M_S4ST4;
			}
			
			// Set pin configuration
			P_SW2 = (P_SW2 & ~M_S4_S) | ((pinSwitch << P_S4_S) & M_S4_S);
			
			// Set UART mode and clear interrupt flag
			S4CON = scon;
			
			// Enable serial port interrupt
			IE2 |= M_S4IE;
			break;
#endif // HAL_UARTS >= 3
		}
		
		_uartTransmitBuffer(uart)->status = STATUS_CLEAR;
	}
	
	return rc;
}

#ifdef WARNING_85
	#pragma restore
	#undef WARNING_85
#endif // WARNING_85

INTERRUPT(uart1_isr, UART1_INTERRUPT) {
	uint8_t c;
	
	EA = 0;
	
	if (S1CON & M_UART_TXIF) {
		S1CON &= ~M_UART_TXIF;
		
		if (fifoRead(&_UART1_transmitBuffer, &c, 1)) {
			S1BUF = c;
		} else {
			_UART1_transmitBuffer.status = STATUS_CLEAR;
		}
	}

	if (S1CON & M_UART_RXIF) {
		S1CON &= ~M_UART_RXIF;
		c = S1BUF;
		fifoWrite(&_UART1_receiveBuffer, &c, 1);
	}
	
	EA = 1;
}

#if HAL_UARTS >= 2
	INTERRUPT(uart2_isr, UART2_INTERRUPT) {
		uint8_t c;
		
		EA = 0;
		
		if (S2CON & M_UART_TXIF) {
			S2CON &= ~M_UART_TXIF;
			
			if (fifoRead(&_UART2_transmitBuffer, &c, 1)) {
				S2BUF = c;
			} else {
				_UART2_transmitBuffer.status = STATUS_CLEAR;
			}
		}

		if (S2CON & M_UART_RXIF) {
			S2CON &= ~M_UART_RXIF;
			c = S2BUF;
			fifoWrite(&_UART2_receiveBuffer, &c, 1);
		}
		
		EA = 1;
	}
#endif // HAL_UARTS >= 2

#if HAL_UARTS >= 3
	INTERRUPT(uart3_isr, UART3_INTERRUPT) {
		uint8_t c;
		
		EA = 0;
		
		if (S3CON & M_UART_TXIF) {
			S3CON &= ~M_UART_TXIF;
			
			if (fifoRead(&_UART3_transmitBuffer, &c, 1)) {
				S3BUF = c;
			} else {
				_UART3_transmitBuffer.status = STATUS_CLEAR;
			}
		}

		if (S3CON & M_UART_RXIF) {
			S3CON &= ~M_UART_RXIF;
			c = S3BUF;
			fifoWrite(&_UART3_receiveBuffer, &c, 1);
		}
		
		EA = 1;
	}

	INTERRUPT(uart4_isr, UART4_INTERRUPT) {
		uint8_t c;
		
		EA = 0;
		
		if (S4CON & M_UART_TXIF) {
			S4CON &= ~M_UART_TXIF;
			
			if (fifoRead(&_UART4_transmitBuffer, &c, 1)) {
				S4BUF = c;
			} else {
				_UART4_transmitBuffer.status = STATUS_CLEAR;
			}
		}

		if (S4CON & M_UART_RXIF) {
			S4CON &= ~M_UART_RXIF;
			c = S4BUF;
			fifoWrite(&_UART4_receiveBuffer, &c, 1);
		}
		
		EA = 1;
	}
#endif // HAL_UARTS >= 3

bool uartGetBlock(Uart uart, uint8_t *data, uint8_t size, BlockingOperation blocking) {
	bool rc = true;
	FifoState *buffer = _uartReceiveBuffer(uart);
	
	do {
		EA = 0;
		rc = fifoRead(buffer, data, size);
		EA = 1;
	} while (blocking == BLOCKING && !rc);
	
	return rc;
}

bool uartSendBlock(Uart uart, const uint8_t *data, uint8_t size, BlockingOperation blocking) {
	FifoState *buffer = _uartTransmitBuffer(uart);
	bool rc = true;
	
	do {
		EA = 0;
		rc = fifoWrite(buffer, data, size);
		EA = 1;
	} while (blocking == BLOCKING && !rc);
	
	if (rc && buffer->status == STATUS_CLEAR) {
		buffer->status = STATUS_SENDING;
		uint8_t data;
		fifoRead(buffer, &data, 1);
		
		switch (uart) {
		case UART1:
			S1BUF = data;
			break;
		
#if HAL_UARTS >= 2
		case UART2:
			S2BUF = data;
			break;
#endif // HAL_UARTS >= 2
		
#if HAL_UARTS >= 3
		case UART3:
			S3BUF = data;
			break;
		
		case UART4:
			S4BUF = data;
			break;
#endif // HAL_UARTS >= 3
		}
	}
	
	return rc;
}
