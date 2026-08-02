#include "project-defs.h"
#include "glow-enhpwm.h"
#include <enhpwm-hal.h>

#define PWM_COUNTER_VALUE 32767u

// Gamma correction table generated using https://github.com/victornpb/gamma-table-generator
// (gamma = 2.0, steps = 32, max. value = 32767).
// Replaced 0 with 1 and 32767 with 32766 to smoothen the glow.
static const uint16_t PWM_GLOW_GRADIENT[] = {
	1, 34, 136, 307, 546, 852, 1227, 1671, 
	2182, 2762, 3410, 4126, 4910, 5762, 6683, 7672, 
	8729, 9854, 11047, 12309, 13639, 15037, 16503, 18037, 
	19640, 21310, 23049, 24857, 26732, 28675, 30687, 32766, 
};

#define PWM_GLOW_STEPS (sizeof(PWM_GLOW_GRADIENT) / sizeof(PWM_GLOW_GRADIENT[0]))

static int8_t _pwmGlowStep = 0;
static int8_t _pwmGlowIncrement = 1;

void pwmGlowUpdateDutyCycle() {
	// For better results:
	// - If your LED has its anode connected to the GPIO and its cathode to ground,
	// you want to use: pwmSetFlipPoints(PWM_GLOW_CHANNEL, 0, PWM_COUNTER_VALUE - PWM_GLOW_GRADIENT[_pwmGlowStep]);
	// - If your LED has its anode connected to VCC and its cathode to the GPIO,
	// you want to use: pwmSetFlipPoints(PWM_GLOW_CHANNEL, 0, PWM_GLOW_GRADIENT[_pwmGlowStep]);
	pwmSetFlipPoints(PWM_GLOW_CHANNEL, 0, PWM_GLOW_GRADIENT[_pwmGlowStep]);
	
	int8_t newStep = _pwmGlowStep + _pwmGlowIncrement;
	
	if (newStep < 0 || newStep >= PWM_GLOW_STEPS) {
		_pwmGlowIncrement = -_pwmGlowIncrement;
	}
	
	_pwmGlowStep += _pwmGlowIncrement;
}

void pwmGlowInitialise() {
	pwmStartCounter(
		PWM_SYSCLK_DIV_7, 
		PWM_COUNTER_VALUE, 
		DISABLE_INTERRUPT
	);
	pwmConfigureOutput(
		PWM_GLOW_CHANNEL, 
		PWM_GLOW_PIN_CONFIG, 
		GPIO_PUSH_PULL_MODE
	);
	pwmStartChannel(
		PWM_GLOW_CHANNEL, 
		OUTPUT_LOW, 
		PWM_INTERRUPT_EVENT_NONE, 
		0,
		PWM_COUNTER_VALUE - PWM_GLOW_GRADIENT[0]
	);
}
