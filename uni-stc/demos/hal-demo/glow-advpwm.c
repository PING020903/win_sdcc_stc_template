#include "project-defs.h"
#include "glow-advpwm.h"
#include <advpwm-hal.h>

// We want the PWM counter to count from 0 to 65535, i.e. 65536 steps.
#define PWM_COUNTER_VALUE 65535u

// Gamma correction table generated using https://github.com/victornpb/gamma-table-generator
// (gamma = 2.0, steps = 32, max. value = 65535).
// Replaced 0 with 1 and 65535 with 65534 to smoothen the glow.
static const uint16_t PWM_GLOW_GRADIENT[] = {
	1, 68, 273, 614, 1091, 1705, 2455, 3342, 
	4364, 5524, 6819, 8252, 9820, 11525, 13366, 15344, 
	17458, 19708, 22095, 24618, 27278, 30074, 33006, 36075, 
	39280, 42622, 46100, 49714, 53465, 57352, 61375, 65534, 
};

#define PWM_GLOW_STEPS (sizeof(PWM_GLOW_GRADIENT) / sizeof(PWM_GLOW_GRADIENT[0]))

static int8_t _pwmGlowStep = 0;
static int8_t _pwmGlowIncrement = 1;

void pwmGlowUpdateDutyCycle() {
	// For better results:
	// - If your LED has its anode connected to the GPIO and its cathode to ground,
	// you want to use: pwmSetDutyCycle(PWM_GLOW_CHANNEL, PWM_COUNTER_VALUE - PWM_GLOW_GRADIENT[_pwmGlowStep]);
	// - If your LED has its anode connected to VCC and its cathode to the GPIO,
	// you want to use: pwmSetDutyCycle(PWM_GLOW_CHANNEL, PWM_GLOW_GRADIENT[_pwmGlowStep]);
	pwmSetDutyCycle(PWM_GLOW_CHANNEL, PWM_GLOW_GRADIENT[_pwmGlowStep]);
	
	int8_t newStep = _pwmGlowStep + _pwmGlowIncrement;
	
	if (newStep < 0 || newStep >= PWM_GLOW_STEPS) {
		_pwmGlowIncrement = -_pwmGlowIncrement;
	}
	
	_pwmGlowStep += _pwmGlowIncrement;
}

void pwmGlowInitialise() {
	pwmConfigureCounter(
		PWM_GLOW_COUNTER, 
		(uint16_t) (MCU_FREQ / PWM_GLOW_SIGNAL_FREQ / (((unsigned long) PWM_COUNTER_VALUE) + 1ul) - 1), 
		(uint16_t) PWM_COUNTER_VALUE, 
		PWM_FREE_RUNNING, 
		PWM_NO_TRIGGER,
		0, 
		PWM_BUFFERED_UPDATE,
		PWM_CONTINUOUS,
		PWM_EDGE_ALIGNED_UP,
		PWM_DISABLE_ALL_UE,
		DISABLE_INTERRUPT
	);
	
	pwmInitialisePWM(
		PWM_GLOW_CHANNEL, 
		OUTPUT_HIGH, 
		DISABLE_INTERRUPT, 
		PWM_IMMEDIATE_UPDATE,
		PWM_GLOW_GRADIENT[0]
	);
	
	pwmConfigureOutput(
		PWM_GLOW_CHANNEL, 
		PWM_GLOW_PIN_CONFIG, 
		GPIO_PUSH_PULL_MODE,
		PWM_ACTIVE_LOW, 
		PWM_DISABLE_FAULT_CONTROL, 
		OUTPUT_HIGH,
		PWM_OUTPUT_P_ONLY
	);
	
	pwmEnableMainOutput(PWM_GLOW_COUNTER);
	pwmEnableCounter(PWM_GLOW_COUNTER);
}
