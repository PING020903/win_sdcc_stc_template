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
#include "glow-pca.h"
#include <pca-hal.h>

#define PCA_COUNTER_VALUE 255u

// Gamma correction table generated using https://github.com/victornpb/gamma-table-generator
// (gamma = 2.0, steps = 32, max. value = 255).
// Replaced 0 with 1 and 255 with 254 to smoothen the glow.
static const uint8_t PCA_GLOW_GRADIENT[] = {
	1, 1, 1, 2, 4, 7, 10, 13, 
	17, 21, 27, 32, 38, 45, 52, 60, 
	68, 77, 86, 96, 106, 117, 128, 140, 
	153, 166, 179, 193, 208, 223, 239, 254, 
};

#define PCA_GLOW_STEPS (sizeof(PCA_GLOW_GRADIENT) / sizeof(PCA_GLOW_GRADIENT[0]))

static int8_t _pcaGlowStep = 0;
static int8_t _pcaGlowIncrement = 1;

#ifndef PCA_GLOW_ENABLE_INTERRUPT

#pragma save
// Suppress warning "unreferenced function argument"
#pragma disable_warning 85
void pcaOnInterrupt(PCA_Channel channel, uint16_t HAL_PCA_SEGMENT pulseLength) {
}
#pragma restore

#endif

void pcaGlowUpdateDutyCycle() {
	// For better results:
	// - If your LED has its anode connected to the GPIO and its cathode to ground,
	// you want to use: pcaSetDutyCycle(PCA_GLOW_CHANNEL, PCA_GLOW_GRADIENT[_pcaGlowStep]);
	// - If your LED has its anode connected to VCC and its cathode to the GPIO,
	// you want to use: pcaSetDutyCycle(PCA_GLOW_CHANNEL, PCA_COUNTER_VALUE - PCA_GLOW_GRADIENT[_pcaGlowStep]);
	pcaSetDutyCycle(PCA_GLOW_CHANNEL, PCA_COUNTER_VALUE - PCA_GLOW_GRADIENT[_pcaGlowStep]);
	
	int8_t newStep = _pcaGlowStep + _pcaGlowIncrement;
	
	if (newStep < 0 || newStep >= PCA_GLOW_STEPS) {
		_pcaGlowIncrement = -_pcaGlowIncrement;
	}
	
	_pcaGlowStep += _pcaGlowIncrement;
}

void pcaGlowInitialise() {
	pcaStartCounter(
		PCA_TIMER0, 
		FREE_RUNNING, 
		DISABLE_INTERRUPT, 
		PCA_GLOW_PIN_CONFIG
	);
	pcaConfigureOutput(
		PCA_GLOW_CHANNEL, 
		GPIO_BIDIRECTIONAL_MODE
	);
	pcaStartPwm(
		PCA_GLOW_CHANNEL, 
		MAKE_PCA_PWM_BITS(PCA_GLOW_PWM_BITS), 
		PCA_EDGE_NONE, 
		PCA_COUNTER_VALUE - PCA_GLOW_GRADIENT[0]
	);
}
