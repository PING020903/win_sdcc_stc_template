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
#include <advpwm-hal.h>
#include <gpio-hal.h>

/**
 * @file advpwm-hal.c
 * 
 * 16-bit advanced PWM abstraction implementation.
 */

typedef enum {
	USAGE_UNUSED,
	USAGE_PWM,
	USAGE_ENCODER,
	USAGE_CAPTURE,
} PWM_ChannelUsage;

static PWM_ChannelUsage HAL_PWM_SEGMENT _channelUsage[] = {
	USAGE_UNUSED, 
#if HAL_PWM_CHANNELS > 1
	USAGE_UNUSED, 
#endif
#if HAL_PWM_CHANNELS > 2
	USAGE_UNUSED, USAGE_UNUSED, 
#endif
#if HAL_PWM_CHANNELS > 4
	USAGE_UNUSED, USAGE_UNUSED, USAGE_UNUSED, USAGE_UNUSED, 
#endif
};

static uint16_t HAL_PWM_SEGMENT _counterOverflow[] = { 0, 0 };

typedef union {
	struct {
		uint16_t counter;
		uint16_t overflow;
	} time;
	uint32_t value;
} PWM_ChannelData;

typedef union {
	struct {
		uint8_t low;
		uint8_t high;
	} byte;
	uint16_t value;
} PWM_CounterValue;

static PWM_ChannelData HAL_PWM_SEGMENT _channelLastCount[HAL_PWM_CHANNELS];

#define PIN_CONFIG_MAX 3
#define UNSUPPORTED_PIN_SWITCH 0xff

static const uint8_t _channelPinConfigurations[][12] = {
#if MCU_PINS == 64
	//PWM1P PWM1N PWM2P PWM2N PWM3P PWM3N PWM4P PWM4N PWM5  PWM6  PWM7  PWM8
	{ 0x10, 0x11, 0x54, 0x13, 0x14, 0x15, 0x16, 0x17, 0x20, 0x21, 0x22, 0x23, },
	{ 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x17, 0x54, 0x33, 0x34, },
	{ 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x00, 0x01, 0x02, 0x03, },
	{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x34, 0x33, 0x74, 0x75, 0x76, 0x77, },
#elif MCU_PINS > 20
	// LQFP-32, DIP-40, LQFP-48 packages: no P6, no P7
	//PWM1P PWM1N PWM2P PWM2N PWM3P PWM3N PWM4P PWM4N PWM5  PWM6  PWM7  PWM8
	{ 0x10, 0x11, 
	#ifdef GPIO_NO_P12
	              0x54, 
	#else
	              0x12, 
	#endif
	                    0x13, 0x14, 0x15, 0x16, 0x17, 0x20, 0x21, 0x22, 0x23, },
	{ 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x17, 0x54, 0x33, 0x34, },
	{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x01, 0x02, 0x03, },
	{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x34, 0x33, 0xff, 0xff, 0xff, 0xff, },
#elif MCU_PINS == 20
	// TSSOP-20 packages: only P1, P3 and P5.4
	//PWM1P PWM1N PWM2P PWM2N PWM3P PWM3N PWM4P PWM4N PWM5  PWM6  PWM7  PWM8
	{ 0x10, 0x11, 
	#ifdef GPIO_NO_P12
	              0x54, 
	#else
	              0x12, 
	#endif
	                    0x13, 0x14, 0x15, 0x16, 0x17, 0xff, 0xff, 0xff, 0xff, },
	{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x17, 0x54, 0x33, 0x34, },
	{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, },
	{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x34, 0x33, 0xff, 0xff, 0xff, 0xff, },
#else
	// STC8H1K08/17-36I-SOP16: only part of P1, P3 and P5.4
	//PWM1P PWM1N PWM2P PWM2N PWM3P PWM3N PWM4P PWM4N PWM5  PWM6  PWM7  PWM8
	{ 0x10, 0x11, 0xff, 0xff, 0xff, 0xff, 0x16, 0x17, 0xff, 0xff, 0xff, 0xff, },
	{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x17, 0x54, 0x33, 0x34, },
	{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, },
	{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x34, 0x33, 0xff, 0xff, 0xff, 0xff, },
#endif
};

static const uint8_t _triggerPinConfigurations[][2] = {
	{ 0x32, 0x32, },
	{ 0x41, 0x06, },
	{ 0x73, 0xff, },
	{ 0xff, 0xff, }, // 0xff means: Unsupported
};

static const uint8_t _faultPinConfigurations[][2] = {
	{ 0x35, 0x35, },
	{ 0x99, 0x99, }, // 0x99 means: Comparator output
};

static uint8_t _getChannelPin(PWM_Channel channel, uint8_t pinSwitch, uint8_t offset) {
	if (pinSwitch >= PIN_CONFIG_MAX) {
		pinSwitch = 0;
	}
	
	uint8_t index = channel;
	
#if HAL_PWM_CHANNELS > 4
	if (channel > PWM_Channel4) {
		index--;
	}
	
	if (channel > PWM_Channel5) {
		index--;
	}
	
	if (channel > PWM_Channel6) {
		index--;
	}
	
	if (channel < PWM_Channel4) {
#endif
		index += offset;
#if HAL_PWM_CHANNELS > 4
	}
#endif
	
	return _channelPinConfigurations[pinSwitch][index];
}

static bool _configurePin(uint8_t ioPin, GpioPinMode pinMode) {
	GpioPort port = (GpioPort) (ioPin >> 4);
	GpioPin pin = (GpioPin) (ioPin & 0x0f);
	bool rc = (port < 8 && pin < 8);
	
	if (rc) {
		GpioConfig pinConfig = GPIO_PIN_CONFIG(port, pin, pinMode);
		gpioConfigure(&pinConfig);
	}
	
	return rc;
}

static void _applyPinSwitch(PWM_Channel channel, uint8_t pinSwitch) {
	if (pinSwitch >= PIN_CONFIG_MAX) {
		pinSwitch = 0;
	}
	
#if HAL_PWM_CHANNELS > 4
	if (channel < PWM_Channel4) {
#endif
		PWMA_PS = (PWMA_PS & ~(3 << channel)) | (pinSwitch << channel);
#if HAL_PWM_CHANNELS > 4
	} else {
		channel -= 8;
		PWMB_PS = (PWMB_PS & ~(3 << channel)) | (pinSwitch << channel);
	}
#endif
}

static void _enableOutput(PWM_Channel channel, uint8_t offset) {
#if HAL_PWM_CHANNELS > 4
	if (channel < PWM_Channel4) {
#endif
		PWMA_ENO |= 1 << (channel + offset);
#if HAL_PWM_CHANNELS > 4
	} else {
		PWMB_ENO |= 1 << (channel - PWM_Channel4);
	}
#endif
}

static void _enableChannel(PWM_Channel channel, uint8_t offset, PWM_Polarity polarity) {
	bool reg2 = false;
	uint8_t bitVal = (polarity << 1) | 1;
	
#if HAL_PWM_CHANNELS > 4
	if (channel < PWM_Channel4) {
#endif
		uint8_t bitPos = (channel + offset) << 1;
		
		if (bitPos > 7) {
			bitPos -= 8;
			reg2 = true;
		}
		
		uint8_t bitMask = ~(3 << bitPos);
		bitVal <<= bitPos;
		
		if (reg2) {
			PWMA_CCER2 = (PWMA_CCER2 & bitMask) | bitVal;
		} else {
			PWMA_CCER1 = (PWMA_CCER1 & bitMask) | bitVal;
		}
#if HAL_PWM_CHANNELS > 4
	} else {
		uint8_t bitPos = (channel - PWM_Channel4) << 1;
		
		if (bitPos > 7) {
			bitPos -= 8;
			reg2 = true;
		}
		
		uint8_t bitMask = ~(3 << bitPos);
		bitVal <<= bitPos;
		
		if (reg2) {
			PWMB_CCER2 = (PWMB_CCER2 & bitMask) | bitVal;
		} else {
			PWMB_CCER1 = (PWMB_CCER1 & bitMask) | bitVal;
		}
	}
#endif
}

static void _enableFaultControl(PWM_Channel channel, uint8_t offset, PWM_FaultControl faultControl, OutputLevel idleLevel) {
	if (faultControl == PWM_ENABLE_FAULT_CONTROL) {
#if HAL_PWM_CHANNELS > 4
		if (channel < PWM_Channel4) {
#endif
			PWMA_IOAUX |= 1 << (channel + offset);
			PWMA_OISR |= idleLevel << (channel + offset);
#if HAL_PWM_CHANNELS > 4
		} else {
			PWMB_IOAUX |= 1 << (channel - PWM_Channel4);
			PWMB_OISR |= idleLevel << (channel - PWM_Channel4);
		}
#endif
	}
}

static void _closeChannel(PWM_Channel channel) {
	switch (channel) {
	case PWM_Channel0:
		PWMA_CCER1 &= 0xf0;
		break;
#if HAL_PWM_CHANNELS > 1
	case PWM_Channel1:
		PWMA_CCER1 &= 0x0f;
		break;
#endif
#if HAL_PWM_CHANNELS > 2
	case PWM_Channel2:
		PWMA_CCER2 &= 0xf0;
		break;
	case PWM_Channel3:
		PWMA_CCER2 &= 0x0f;
		break;
#endif
#if HAL_PWM_CHANNELS > 4
	case PWM_Channel4:
		PWMB_CCER1 &= 0xf0;
		break;
	case PWM_Channel5:
		PWMB_CCER1 &= 0x0f;
		break;
	case PWM_Channel6:
		PWMB_CCER2 &= 0xf0;
		break;
	case PWM_Channel7:
		PWMB_CCER2 &= 0x0f;
		break;
#endif
	}
}

void pwmConfigureOutput(
	PWM_Channel channel, 
	uint8_t pinSwitch, 
	GpioPinMode pinMode, 
	PWM_Polarity polarity, 
	PWM_FaultControl faultControl,
	OutputLevel idleLevel,
	PWM_OutputEnable outputEnable
) {
	if (pinMode == GPIO_HIGH_IMPEDANCE_MODE) {
		pinMode = GPIO_PUSH_PULL_MODE;
	}
	
	bool ok = false;
	
	if (channel > PWM_Channel3 || (outputEnable & PWM_OUTPUT_P_ONLY)) {
		if (_configurePin(_getChannelPin(channel, pinSwitch, 0), pinMode)) {
			_enableOutput(channel, 0);
			_enableChannel(channel, 0, polarity);
			_enableFaultControl(channel, 0, faultControl, idleLevel);
			ok = true;
		}
	}
	
	if (channel <= PWM_Channel3 && (outputEnable & PWM_OUTPUT_N_ONLY)) {
		if (_configurePin(_getChannelPin(channel, pinSwitch, 1), pinMode)) {
			_enableOutput(channel, 1);
			_enableChannel(channel, 1, polarity);
			_enableFaultControl(channel, 1, faultControl, idleLevel);
			ok = true;
		}
	}
	
	if (ok) {
		_applyPinSwitch(channel, pinSwitch);
	}
}

#ifdef HAL_PWM_API_DISABLE
static void _disableOutput(PWM_Channel channel, uint8_t offset) {
#if HAL_PWM_CHANNELS > 4
	if (channel < PWM_Channel4) {
#endif
		PWMA_ENO &= ~(1 << (channel + offset));
#if HAL_PWM_CHANNELS > 4
	} else {
		PWMB_ENO &= ~(1 << (channel - PWM_Channel4));
	}
#endif
}

void pwmEnableChannelOutput(PWM_Channel channel, PWM_OutputEnable outputEnable) {
	if (channel > PWM_Channel3 || (outputEnable & PWM_OUTPUT_P_ONLY)) {
		_enableOutput(channel, 0);
	}
	
	if (channel <= PWM_Channel3 && (outputEnable & PWM_OUTPUT_N_ONLY)) {
		_enableOutput(channel, 1);
	}
}

void pwmDisableChannelOutput(PWM_Channel channel, PWM_OutputEnable outputEnable) {
	if (channel > PWM_Channel3 || (outputEnable & PWM_OUTPUT_P_ONLY)) {
		_disableOutput(channel, 0);
	}
	
	if (channel <= PWM_Channel3 && (outputEnable & PWM_OUTPUT_N_ONLY)) {
		_disableOutput(channel, 1);
	}
}
#endif // HAL_PWM_API_DISABLE

#ifdef HAL_PWM_CALCULATE_PARAMETERS
uint16_t pwmCalculatePrescalerAndReloadValue(uint32_t targetFreq, uint16_t *reloadValue) {
	uint16_t prescaler = 1;
	uint32_t freqRatio = MCU_FREQ / targetFreq;
	
	if (freqRatio > PWM_COUNTER_SCALE) {
		prescaler = freqRatio / PWM_COUNTER_SCALE;
		
		if (MCU_FREQ % prescaler) {
			prescaler++;
		}

		freqRatio /= prescaler;
	}
	
	prescaler--;
	*reloadValue = freqRatio - 1ul;
	
	return prescaler;
}
#endif // HAL_PWM_CALCULATE_PARAMETERS

void pwmConfigureCounter(
	PWM_Counter counter, 
	uint16_t prescaler, 
	uint16_t reloadValue, 
	PWM_CounterMode mode, 
	PWM_TriggerSource trigger,
	uint8_t repeatCount, 
	PWM_RegisterUpdate registerUpdateMode,
	PWM_CounterRunMode counterRunMode,
	PWM_CounterDirection counterDirection,
	PWM_UpdateEventEnable updateEventEnable,
	InterruptEnable comInterrupt
) {
	uint8_t slaveMode = mode | (trigger << 4);
	
	uint8_t cr1 = registerUpdateMode << P_ARPE
		| counterRunMode << P_OPM
		| counterDirection << P_DIR
		| updateEventEnable << P_UDIS;
	
	uint8_t ier = (comInterrupt == ENABLE_INTERRUPT ? M_COMIE : 0)
		| (updateEventEnable != PWM_DISABLE_ALL_UE ? M_UIE : 0);
	
	
	switch (counter) {
	case PWM_COUNTER_A:
		PWMA_SMCR = slaveMode;
		PWMA_CR1 = cr1;
		PWMA_IER = (PWMA_IER & ~(M_COMIE | M_UIE)) | ier;
		PWMA_RCR = repeatCount;
		PWMA_PSCRH = prescaler >> 8;
		PWMA_PSCRL = prescaler;
		PWMA_ARRH = reloadValue >> 8;
		PWMA_ARRL = reloadValue;
		break;
	
#if HAL_PWM_CHANNELS > 4
	case PWM_COUNTER_B:
		PWMB_SMCR = slaveMode;
		PWMB_CR1 = cr1;
		PWMB_IER = (PWMB_IER & ~(M_COMIE | M_UIE)) | ier;
		PWMB_RCR = repeatCount;
		PWMB_PSCRH = prescaler >> 8;
		PWMB_PSCRL = prescaler;
		PWMB_ARRH = reloadValue >> 8;
		PWMB_ARRL = reloadValue;
		break;
#endif
	}
}

void pwmEnableMainOutput(PWM_Counter counter) {
	switch (counter) {
	case PWM_COUNTER_A:
		PWMA_BKR |= M_MOE;
		break;
	
#if HAL_PWM_CHANNELS > 4
	case PWM_COUNTER_B:
		PWMB_BKR |= M_MOE;
		break;
#endif
	}
}

void pwmEnableCounter(PWM_Counter counter) {
	switch (counter) {
	case PWM_COUNTER_A:
		PWMA_CR1 |= M_CEN;
		break;
	
#if HAL_PWM_CHANNELS > 4
	case PWM_COUNTER_B:
		PWMB_CR1 |= M_CEN;
		break;
#endif
	}
}

#ifdef HAL_PWM_API_DISABLE
void pwmDisableMainOutput(PWM_Counter counter) {
	switch (counter) {
	case PWM_COUNTER_A:
		PWMA_BKR &= ~M_MOE;
		break;
	
#if HAL_PWM_CHANNELS > 4
	case PWM_COUNTER_B:
		PWMB_BKR &= ~M_MOE;
		break;
#endif
	}
}

void pwmDisableCounter(PWM_Counter counter) {
	switch (counter) {
	case PWM_COUNTER_A:
		PWMA_CR1 &= ~M_CEN;
		break;
	
#if HAL_PWM_CHANNELS > 4
	case PWM_COUNTER_B:
		PWMB_CR1 &= ~M_CEN;
		break;
#endif
	}
}
#endif // HAL_PWM_API_DISABLE

#ifdef HAL_PWM_API_DEAD_TIME
/**
 * @param clockPulses is the duration of the dead time expressed in 
 * number of clock pulses on the input of the prescaler, i.e. sysclk
 * unless you use an external PWM clock.
 */
void pwmConfigureDeadTime(PWM_Counter counter, uint16_t clockPulses) {
	uint8_t dtr = 255;
	
	if (clockPulses < 128) {
		dtr = clockPulses;
	} else if (clockPulses < 255) {
		dtr = ((clockPulses >> 1) - 64) | 0x80;
	} else if (clockPulses < 505) {
		dtr = ((clockPulses >> 3) - 32) | 0xc0;
	} else if (clockPulses < 1009) {
		dtr = ((clockPulses >> 4) - 32) | 0xe0;
	}
	
	switch (counter) {
	case PWM_COUNTER_A:
		PWMA_DTR = dtr;
		break;
	
#if HAL_PWM_CHANNELS > 4
	case PWM_COUNTER_B:
		PWMB_DTR = dtr;
		break;
#endif
	}
}
#endif // HAL_PWM_API_DEAD_TIME

static void _enableChannelInterrupt(PWM_Channel channel) {
#if HAL_PWM_CHANNELS > 4
	if (channel < PWM_Channel4) {
#endif
		PWMA_IER |= 1 << ((channel >> 1) + 1);
#if HAL_PWM_CHANNELS > 4
	} else {
		PWMB_IER |= 1 << (((channel - PWM_Channel4) >> 1) + 1);
	}
#endif
}

void pwmInitialisePWM(
	PWM_Channel channel, 
	OutputLevel initialLevel, 
	InterruptEnable enableInterrupt, 
	PWM_RegisterUpdate registerUpdateMode,
	uint16_t ticks
) {
	// Channel must be closed before writing to CCMRx
	_closeChannel(channel);
	
	pwmSetDutyCycle(channel, ticks);
	
	// Enable/disable preload
	uint8_t ccmr = (registerUpdateMode == PWM_BUFFERED_UPDATE) ? M_OC_PE : 0;
	
	// OC_M == 6 is PWM mode 1, i.e. wave form starts with a high level.
	// OC_M == 7 is PWM mode 2, i.e. wave form starts with a low level.
	ccmr |= (initialLevel == OUTPUT_LOW ? 7 : 6) << P_OC_M;
	
	switch (channel) {
	case PWM_Channel0:
		PWMA_CCMR1 = ccmr;
		break;
#if HAL_PWM_CHANNELS > 1
	case PWM_Channel1:
		PWMA_CCMR2 = ccmr;
		break;
#endif
#if HAL_PWM_CHANNELS > 2
	case PWM_Channel2:
		PWMA_CCMR3 = ccmr;
		break;
	case PWM_Channel3:
		PWMA_CCMR4 = ccmr;
		break;
#endif
#if HAL_PWM_CHANNELS > 4
	case PWM_Channel4:
		PWMB_CCMR1 = ccmr;
		break;
	case PWM_Channel5:
		PWMB_CCMR2 = ccmr;
		break;
	case PWM_Channel6:
		PWMB_CCMR3 = ccmr;
		break;
	case PWM_Channel7:
		PWMB_CCMR4 = ccmr;
		break;
#endif
	}
	
	_channelUsage[channel >> 1] = USAGE_PWM;
	
	if (enableInterrupt == ENABLE_INTERRUPT) {
		_enableChannelInterrupt(channel);
	}
}

#ifdef HAL_PWM_API_STOP
void pwmStopPWM(PWM_Channel channel) {
	// OC_M == 0 freezes PWM output.
	switch (channel) {
	case PWM_Channel0:
		PWMA_CCMR1 = 0;
		break;
#if HAL_PWM_CHANNELS > 1
	case PWM_Channel1:
		PWMA_CCMR2 = 0;
		break;
#endif
#if HAL_PWM_CHANNELS > 2
	case PWM_Channel2:
		PWMA_CCMR3 = 0;
		break;
	case PWM_Channel3:
		PWMA_CCMR4 = 0;
		break;
#endif
#if HAL_PWM_CHANNELS > 4
	case PWM_Channel4:
		PWMB_CCMR1 = 0;
		break;
	case PWM_Channel5:
		PWMB_CCMR2 = 0;
		break;
	case PWM_Channel6:
		PWMB_CCMR3 = 0;
		break;
	case PWM_Channel7:
		PWMB_CCMR4 = 0;
		break;
#endif
	}
}
#endif // HAL_PWM_API_STOP

#ifdef HAL_PWM_API_LOCK
void pwmLockPWM(PWM_Channel channel, OutputLevel outputLevel) {
	// OC_M == 4 sets the PWM output to a low level.
	// OC_M == 5 sets the PWM output to a high level.
	uint8_t ccmr = (outputLevel == OUTPUT_LOW ? 4 : 5) << P_OC_M;
	
	switch (channel) {
	case PWM_Channel0:
		PWMA_CCMR1 = (PWMA_CCMR1 & ~M_OC_M) | ccmr;
		break;
#if HAL_PWM_CHANNELS > 1
	case PWM_Channel1:
		PWMA_CCMR2 = (PWMA_CCMR2 & ~M_OC_M) | ccmr;
		break;
#endif
#if HAL_PWM_CHANNELS > 2
	case PWM_Channel2:
		PWMA_CCMR3 = (PWMA_CCMR3 & ~M_OC_M) | ccmr;
		break;
	case PWM_Channel3:
		PWMA_CCMR4 = (PWMA_CCMR4 & ~M_OC_M) | ccmr;
		break;
#endif
#if HAL_PWM_CHANNELS > 4
	case PWM_Channel4:
		PWMB_CCMR1 = (PWMB_CCMR1 & ~M_OC_M) | ccmr;
		break;
	case PWM_Channel5:
		PWMB_CCMR2 = (PWMB_CCMR2 & ~M_OC_M) | ccmr;
		break;
	case PWM_Channel6:
		PWMB_CCMR3 = (PWMB_CCMR3 & ~M_OC_M) | ccmr;
		break;
	case PWM_Channel7:
		PWMB_CCMR4 = (PWMB_CCMR4 & ~M_OC_M) | ccmr;
		break;
#endif
	}
}
#endif // HAL_PWM_API_LOCK

void pwmSetDutyCycle(PWM_Channel channel, uint16_t ticks) {
	uint8_t ticksH = ticks >> 8;
	uint8_t ticksL = ticks;
	
	switch (channel) {
	case PWM_Channel0:
		PWMA_CCR1H = ticksH;
		PWMA_CCR1L = ticksL;
		break;
#if HAL_PWM_CHANNELS > 1
	case PWM_Channel1:
		PWMA_CCR2H = ticksH;
		PWMA_CCR2L = ticksL;
		break;
#endif
#if HAL_PWM_CHANNELS > 2
	case PWM_Channel2:
		PWMA_CCR3H = ticksH;
		PWMA_CCR3L = ticksL;
		break;
	case PWM_Channel3:
		PWMA_CCR4H = ticksH;
		PWMA_CCR4L = ticksL;
		break;
#endif
#if HAL_PWM_CHANNELS > 4
	case PWM_Channel4:
		PWMB_CCR1H = ticksH;
		PWMB_CCR1L = ticksL;
		break;
	case PWM_Channel5:
		PWMB_CCR2H = ticksH;
		PWMB_CCR2L = ticksL;
		break;
	case PWM_Channel6:
		PWMB_CCR3H = ticksH;
		PWMB_CCR3L = ticksL;
		break;
	case PWM_Channel7:
		PWMB_CCR4H = ticksH;
		PWMB_CCR4L = ticksL;
		break;
#endif
	}
}

#ifdef HAL_PWM_API_FAULT_DETECTION
void pwmConfigureFaultDetection(
	PWM_Counter counter, 
	PWM_FaultTrigger faultTrigger, 
	PWM_Polarity faultPolarity, 
	PWM_FaultResponse faultResponse, 
	PWM_FaultResume faultResume, 
	InterruptEnable enableInterrupt
) {
	uint8_t pinSwitch = (faultTrigger == PWM_FAULT_COMPARATOR) ? 1 : 0;
	uint8_t pin = _faultPinConfigurations[pinSwitch][counter];
	_configurePin(pin, GPIO_HIGH_IMPEDANCE_MODE);
	uint8_t bkr = 
		M_MOE
		| (faultResume << P_AOE)
		| (faultPolarity << P_BKP)
		| M_BKE
		| (faultResponse << P_OSSI);
	
#if HAL_PWM_CHANNELS > 4
	switch (counter) {
	case PWM_COUNTER_A:
#endif
		PWMA_ETRPS = (PWMA_ETRPS & ~M_BRK_PS) | (pinSwitch << P_BRK_PS);
		PWMA_BKR = bkr;
		
		if (enableInterrupt == ENABLE_INTERRUPT) {
			PWMA_IER |= M_BIE;
		}
#if HAL_PWM_CHANNELS > 4
		break;
	case PWM_COUNTER_B:
		PWMB_ETRPS = (PWMB_ETRPS & ~M_BRK_PS) | (pinSwitch << P_BRK_PS);
		PWMB_BKR = bkr;
		
		if (enableInterrupt == ENABLE_INTERRUPT) {
			PWMB_IER |= M_BIE;
		}
		break;
	}
#endif
}
#endif // HAL_PWM_API_FAULT_DETECTION

#ifdef HAL_PWM_API_EXTERNAL_TRIGGER
void pwmConfigureExternalTrigger(
	PWM_Counter counter, 
	uint8_t pinSwitch, 
	PWM_TriggerEdge triggerEdge,
	PWM_ExternalClock externalClock,
	PWM_TriggerPrescaler prescaler,
	PWM_Filter filter,
	InterruptEnable enableInterrupt
) {
	if (pinSwitch >= PIN_CONFIG_MAX) {
		pinSwitch = 0;
	}
	
	uint8_t pin = _triggerPinConfigurations[pinSwitch][counter];
	
	if (pin != UNSUPPORTED_PIN_SWITCH) {
		_configurePin(pin, GPIO_HIGH_IMPEDANCE_MODE);
		uint8_t etr = 
			(triggerEdge << P_ETP)
			| (externalClock << P_ECE)
			| (prescaler << P_ETPS)
			| (filter << P_ETF);
		
#if HAL_PWM_CHANNELS > 4
		switch (counter) {
		case PWM_COUNTER_A:
#endif
			PWMA_ETRPS = (PWMA_ETRPS & ~M_ETR_PS) | (pinSwitch << P_ETR_PS);
			PWMA_ETR = etr;
			
			if (enableInterrupt == ENABLE_INTERRUPT) {
				PWMA_IER |= M_TIE;
			}
#if HAL_PWM_CHANNELS > 4
			break;
		case PWM_COUNTER_B:
			PWMB_ETRPS = (PWMB_ETRPS & ~M_ETR_PS) | (pinSwitch << P_ETR_PS);
			PWMB_ETR = etr;
			
			if (enableInterrupt == ENABLE_INTERRUPT) {
				PWMB_IER |= M_TIE;
			}
			break;
		}
#endif
	}
}
#endif // HAL_PWM_API_EXTERNAL_TRIGGER

static void _configureInput(
	PWM_Channel channel, 
	uint8_t pinSwitch, 
	PWM_CaptureEdge captureEdge,
	PWM_CaptureSource captureSource,
	PWM_Filter filter
) {
	_applyPinSwitch(channel, pinSwitch);
	_configurePin(_getChannelPin(channel, pinSwitch, 0), GPIO_HIGH_IMPEDANCE_MODE);
	uint8_t ccmr = (filter << 4) | captureSource;
	
	switch (channel) {
	case PWM_Channel0:
		PWMA_CCMR1 = ccmr;
		break;
#if HAL_PWM_CHANNELS > 1
	case PWM_Channel1:
		PWMA_CCMR2 = ccmr;
		break;
#endif
#if HAL_PWM_CHANNELS > 2
	case PWM_Channel2:
		PWMA_CCMR3 = ccmr;
		break;
	case PWM_Channel3:
		PWMA_CCMR4 = ccmr;
		break;
#endif
#if HAL_PWM_CHANNELS > 4
	case PWM_Channel4:
		PWMB_CCMR1 = ccmr;
		break;
	case PWM_Channel5:
		PWMB_CCMR2 = ccmr;
		break;
	case PWM_Channel6:
		PWMB_CCMR3 = ccmr;
		break;
	case PWM_Channel7:
		PWMB_CCMR4 = ccmr;
		break;
#endif
	}
	
	_enableChannel(channel, 0, captureEdge);
}

#ifdef HAL_PWM_API_QUADRATURE_ENCODER
void pwmInitialiseQuadratureEncoder(
	PWM_Counter counter, 
	uint8_t pinSwitch, 
	PWM_CaptureEdge captureEdge, 
	PWM_Filter filter
) {
#if HAL_PWM_CHANNELS > 4
	PWM_Channel firstChannel = (counter == PWM_COUNTER_A) ? PWM_Channel0 : PWM_Channel4;
#else
	PWM_Channel firstChannel = PWM_Channel0;
#endif
	PWM_Channel secondChannel = firstChannel + 2;
	
	pwmConfigureCounter(
		counter, 
		0, // Prescaler
		0xffffUL, // Auto-reload
		PWM_QUADRATURE_ENCODER, 
		PWM_NO_TRIGGER,
		0, // Repeat counter
		PWM_IMMEDIATE_UPDATE,
		PWM_CONTINUOUS,
		PWM_EDGE_ALIGNED_UP,
		PWM_DISABLE_ALL_UE,
		DISABLE_INTERRUPT
	);

	_configureInput(
		firstChannel, 
		pinSwitch,
		captureEdge,
		PWM_CAPTURE_SAME_PIN,
		filter
	);

	_configureInput(
		secondChannel, 
		pinSwitch,
		captureEdge,
		PWM_CAPTURE_SAME_PIN,
		filter
	);
	
	_channelUsage[firstChannel >> 1] = USAGE_ENCODER;
	_channelUsage[secondChannel >> 1] = USAGE_ENCODER;
	
	// We only want interrupts on the first channel.
	_enableChannelInterrupt(firstChannel);
	
	pwmEnableCounter(counter);
}
#endif // HAL_PWM_API_QUADRATURE_ENCODER

void pwmInitialiseCapture(
	PWM_Channel channel, 
	uint8_t pinSwitch, 
	PWM_CaptureEdge captureEdge, 
	PWM_CaptureSource captureSource,
	PWM_Filter filter,
	PWM_CaptureReference reference
) {
	_configureInput(
		channel, 
		pinSwitch,
		captureEdge,
		captureSource,
		filter
	);

	uint8_t channelIndex = channel >> 1;
	_channelUsage[channelIndex] = USAGE_CAPTURE | reference;
	_channelLastCount[channelIndex].value = 0ul;
	_enableChannelInterrupt(channel);
}

#if !defined(HAL_PWM_NO_COUNTER_INT_HANDLER) || !defined(HAL_PWM_NO_CHANNEL_INT_HANDLER)

INTERRUPT(pwmA_isr, PWMA_INTERRUPT) {
#ifndef HAL_PWM_NO_CHANNEL_INT_HANDLER
	uint8_t channel = 255;
#endif
#ifndef HAL_PWM_NO_COUNTER_INT_HANDLER
	uint8_t event = 255;
#endif
	
	if (PWMA_SR1 & M_CC1IF) {
		PWMA_SR1 &= ~M_CC1IF;
#ifndef HAL_PWM_NO_CHANNEL_INT_HANDLER
		channel = PWM_Channel0;
#endif
	}
	
#if HAL_PWM_CHANNELS > 1
	if (PWMA_SR1 & M_CC2IF) {
		PWMA_SR1 &= ~M_CC2IF;
#ifndef HAL_PWM_NO_CHANNEL_INT_HANDLER
		channel = PWM_Channel1;
#endif
	}
#endif
	
#if HAL_PWM_CHANNELS > 2
	if (PWMA_SR1 & M_CC3IF) {
		PWMA_SR1 &= ~M_CC3IF;
#ifndef HAL_PWM_NO_CHANNEL_INT_HANDLER
		channel = PWM_Channel2;
#endif
	}
	
	if (PWMA_SR1 & M_CC4IF) {
		PWMA_SR1 &= ~M_CC4IF;
#ifndef HAL_PWM_NO_CHANNEL_INT_HANDLER
		channel = PWM_Channel3;
#endif
	}
#endif
	
	if (PWMA_SR1 & M_TIF) {
		PWMA_SR1 &= ~M_TIF;
#ifndef HAL_PWM_NO_COUNTER_INT_HANDLER
		event = PWM_INTERRUPT_TRIGGER;
#endif
	}
	
	if (PWMA_SR1 & M_COMIF) {
		PWMA_SR1 &= ~M_COMIF;
#ifndef HAL_PWM_NO_COUNTER_INT_HANDLER
		event = PWM_INTERRUPT_COM;
#endif
	}
	
	if (PWMA_SR1 & M_UIF) {
		PWMA_SR1 &= ~M_UIF;
#ifndef HAL_PWM_NO_COUNTER_INT_HANDLER
		event = PWM_INTERRUPT_UPDATE;
#endif
		_counterOverflow[PWM_COUNTER_A]++;
	}
	
	if (PWMA_SR1 & M_BIF) {
		// Note: the brake interrupt flag can only be cleared
		// if the brake input is inactive, so it may have to
		// be cleared outside of the ISR.
		PWMA_SR1 &= ~M_BIF;
#ifndef HAL_PWM_NO_COUNTER_INT_HANDLER
		event = PWM_INTERRUPT_FAULT;
#endif
	}
	
#ifndef HAL_PWM_NO_CHANNEL_INT_HANDLER
	if (channel != 255) {
		uint8_t channelIndex = channel >> 1;
		PWM_ChannelUsage usage = _channelUsage[channelIndex];
		
		switch (usage) {
		case USAGE_UNUSED:
			break;
		
		case USAGE_PWM:
			pwmOnChannelInterrupt(channel, 0);
			break;
		
		case USAGE_ENCODER:
			pwmOnChannelInterrupt(channel, PWMA_CR1 & M_DIR);
			break;
		
		default: { // Capture
				PWM_CounterValue counter;
				counter.value = 0;
				
				switch (channel) {
				case PWM_Channel0:
					counter.byte.high = PWMA_CCR1H;
					counter.byte.low = PWMA_CCR1L;
					break;
#if HAL_PWM_CHANNELS > 1
				case PWM_Channel1:
					counter.byte.high = PWMA_CCR2H;
					counter.byte.low = PWMA_CCR2L;
					break;
#endif
#if HAL_PWM_CHANNELS > 2
				case PWM_Channel2:
					counter.byte.high = PWMA_CCR3H;
					counter.byte.low = PWMA_CCR3L;
					break;
				case PWM_Channel3:
					counter.byte.high = PWMA_CCR4H;
					counter.byte.low = PWMA_CCR4L;
					break;
#endif
				}
				
				uint8_t refIndex = channelIndex;
				
#if HAL_PWM_CHANNELS > 1
				if (usage & PWM_REFERENCE_OTHER_PIN) {
					switch (channel) {
					case PWM_Channel0:
						refIndex = PWM_Channel1 >> 1;
						break;
					case PWM_Channel1:
						refIndex = PWM_Channel0 >> 1;
						break;
	#if HAL_PWM_CHANNELS > 2
					case PWM_Channel2:
						refIndex = PWM_Channel3 >> 1;
						break;
					case PWM_Channel3:
						refIndex = PWM_Channel2 >> 1;
						break;
	#endif // HAL_PWM_CHANNELS > 2
					}
				}
#endif // HAL_PWM_CHANNELS > 1
				
				PWM_ChannelData channelNewCount, elapsed;
				channelNewCount.time.overflow = _counterOverflow[PWM_COUNTER_A];
				channelNewCount.time.counter = counter.value;
				elapsed.value = channelNewCount.value - _channelLastCount[refIndex].value;
				
				_channelLastCount[channelIndex].value = channelNewCount.value;
				pwmOnChannelInterrupt(channel, elapsed.time.counter);
			}
			break;
		}
	}
#endif
	
#ifndef HAL_PWM_NO_COUNTER_INT_HANDLER
	if (event != 255) {
		pwmOnCounterInterrupt(PWM_COUNTER_A, event);
	}
#endif // HAL_PWM_NO_COUNTER_INT_HANDLER
}

#if HAL_PWM_CHANNELS > 4
INTERRUPT(pwmB_isr, PWMB_INTERRUPT) {
#ifndef HAL_PWM_NO_CHANNEL_INT_HANDLER
	uint8_t channel = 255;
#endif
#ifndef HAL_PWM_NO_COUNTER_INT_HANDLER
	uint8_t event = 255;
#endif
	
	if (PWMB_SR1 & M_CC5IF) {
		PWMB_SR1 &= ~M_CC5IF;
#ifndef HAL_PWM_NO_CHANNEL_INT_HANDLER
		channel = PWM_Channel4;
#endif
	}
	
	if (PWMB_SR1 & M_CC6IF) {
		PWMB_SR1 &= ~M_CC6IF;
#ifndef HAL_PWM_NO_CHANNEL_INT_HANDLER
		channel = PWM_Channel5;
#endif
	}
	
	if (PWMB_SR1 & M_CC7IF) {
		PWMB_SR1 &= ~M_CC7IF;
#ifndef HAL_PWM_NO_CHANNEL_INT_HANDLER
		channel = PWM_Channel6;
#endif
	}
	
	if (PWMB_SR1 & M_CC8IF) {
		PWMB_SR1 &= ~M_CC8IF;
#ifndef HAL_PWM_NO_CHANNEL_INT_HANDLER
		channel = PWM_Channel7;
#endif
	}
	
	if (PWMB_SR1 & M_TIF) {
		PWMB_SR1 &= ~M_TIF;
#ifndef HAL_PWM_NO_COUNTER_INT_HANDLER
		event = PWM_INTERRUPT_TRIGGER;
#endif
	}
	
	if (PWMB_SR1 & M_COMIF) {
		PWMB_SR1 &= ~M_COMIF;
#ifndef HAL_PWM_NO_COUNTER_INT_HANDLER
		event = PWM_INTERRUPT_COM;
#endif
	}
	
	if (PWMB_SR1 & M_UIF) {
		PWMB_SR1 &= ~M_UIF;
#ifndef HAL_PWM_NO_COUNTER_INT_HANDLER
		event = PWM_INTERRUPT_UPDATE;
#endif
		_counterOverflow[PWM_COUNTER_B]++;
	}
	
	if (PWMB_SR1 & M_BIF) {
		// Note: the brake interrupt flag can only be cleared
		// if the brake input is inactive, so it may have to
		// be cleared outside of the ISR.
		PWMB_SR1 &= ~M_BIF;
#ifndef HAL_PWM_NO_COUNTER_INT_HANDLER
		event = PWM_INTERRUPT_FAULT;
#endif
	}
	
#ifndef HAL_PWM_NO_CHANNEL_INT_HANDLER
	if (channel != 255) {
		uint8_t channelIndex = channel >> 1;
		PWM_ChannelUsage usage = _channelUsage[channelIndex];
		
		switch (usage) {
		case USAGE_UNUSED:
			break;
		
		case USAGE_PWM:
			pwmOnChannelInterrupt(channel, 0);
			break;
		
		case USAGE_ENCODER:
			pwmOnChannelInterrupt(channel, PWMB_CR1 & M_DIR);
			break;
		
		default: { // Capture
				PWM_CounterValue counter;
				counter.value = 0;
				
				switch (channel) {
				case PWM_Channel4:
					counter.byte.high = PWMB_CCR1H;
					counter.byte.low = PWMB_CCR1L;
					break;
				case PWM_Channel5:
					counter.byte.high = PWMB_CCR2H;
					counter.byte.low = PWMB_CCR2L;
					break;
				case PWM_Channel6:
					counter.byte.high = PWMB_CCR3H;
					counter.byte.low = PWMB_CCR3L;
					break;
				case PWM_Channel7:
					counter.byte.high = PWMB_CCR4H;
					counter.byte.low = PWMB_CCR4L;
					break;
				}
				
				uint8_t refIndex = channelIndex;
				
				if (usage & PWM_REFERENCE_OTHER_PIN) {
					switch (channel) {
					case PWM_Channel4:
						refIndex = PWM_Channel5 >> 1;
						break;
					case PWM_Channel5:
						refIndex = PWM_Channel4 >> 1;
						break;
					case PWM_Channel6:
						refIndex = PWM_Channel7 >> 1;
						break;
					case PWM_Channel7:
						refIndex = PWM_Channel6 >> 1;
						break;
					}
				}
				
				PWM_ChannelData channelNewCount, elapsed;
				channelNewCount.time.overflow = _counterOverflow[PWM_COUNTER_B];
				channelNewCount.time.counter = counter.value;
				elapsed.value = channelNewCount.value - _channelLastCount[refIndex].value;
				
				_channelLastCount[channelIndex].value = channelNewCount.value;
				pwmOnChannelInterrupt(channel, elapsed.time.counter);
			}
			break;
		}
	}
#endif
	
#ifndef HAL_PWM_NO_COUNTER_INT_HANDLER
	if (event != 255) {
		pwmOnCounterInterrupt(PWM_COUNTER_B, event);
	}
#endif // HAL_PWM_NO_COUNTER_INT_HANDLER
}
#endif // HAL_PWM_CHANNELS > 4
#endif // !defined(HAL_PWM_NO_COUNTER_INT_HANDLER) || !defined(HAL_PWM_NO_CHANNEL_INT_HANDLER)
