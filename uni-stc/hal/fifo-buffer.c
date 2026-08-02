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
#include <fifo-buffer.h>

/**
 * @file fifo-buffer.c
 * 
 * FIFO circular buffer implementation.
 */

void fifoClear(FifoState *buffer) {
	buffer->length = 0;
	buffer->rIndex = 0;
	buffer->wIndex = 0;
	buffer->status = 0;
}

bool fifoIsFull(const FifoState *fifo) {
	return fifoLength(fifo) == fifo->size;
}

uint8_t fifoBytesFree(const FifoState *fifo) {
	return fifo->size - fifoLength(fifo);
}

/*
 * Without REENTRANT, if a call to fifoWrite() is interrupted and the 
 * ISR calls fifoWrite() on another buffer, the interrupted call could 
 * resume with an invalid address in data, as this argument is stored 
 * in a single memory location. REENTRANT causes arguments to be passed 
 * on the stack, which incurs a small overhead, but is interrupt-safe.
 * 
 * REENTRANT is not needed with functions with only one parameter as it 
 * is passed in registers instead of a memory location.
 */
bool fifoWrite(FifoState *buffer, const void *data, uint8_t count) REENTRANT {
	bool rc = fifoBytesFree(buffer) >= count;
	
	if (rc) {
		uint8_t wIndex = buffer->wIndex;
		
		for (uint8_t n = 0; n < count; n++) {
			if (wIndex == buffer->size) {
				wIndex = 0;
			}
			
			buffer->data[wIndex] = ((const uint8_t *) data)[n];
			wIndex++;
		}
		
		buffer->wIndex = wIndex;
		buffer->length += count;
	}
	
	return rc;
}

bool fifoRead(FifoState *buffer, void *data, uint8_t count) REENTRANT {
	bool rc = fifoLength(buffer) >= count;
	
	if (rc) {
		uint8_t rIndex = buffer->rIndex;
		
		for (uint8_t n = 0; n < count; n++) {
			if (rIndex == buffer->size) {
				rIndex = 0;
			}
			
			// Buffer is not empty, read rIndex character.
			((uint8_t *) data)[n] = buffer->data[rIndex];
			rIndex++;
		}
		
		buffer->rIndex = rIndex;
		buffer->length -= count;
	}
	
	return rc;
}
