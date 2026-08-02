#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// Timers are 16-bit counters.
#define COUNTER_MAX 65536ul
// Prescalers are 8-bit counters.
#define PRESCALER_MAX 256ul

void withoutPrescaler(uint32_t targetDivisor) {
	uint32_t sysclkDivisor = targetDivisor;
	bool freqTooLow = false;
	uint8_t sysclkDiv1 = 1;
	
	if (sysclkDivisor > COUNTER_MAX) {
		if (sysclkDivisor <= (COUNTER_MAX * 12UL)) {
			sysclkDiv1 = 0;
			sysclkDivisor /= 12;
		} else {
			freqTooLow = true;
		}
	}
	
	printf("*** withOUT prescaler:\n");
	
	if (freqTooLow) {
		printf("Target frequency is TOO LOW.\n");
	} else {
		uint32_t effectiveDivisor = sysclkDivisor * (sysclkDiv1 ? 1ul : 12ul);
		printf("Divide system clock by: %d\n", (sysclkDiv1 ? 1 : 12));
		printf("Effective divisor = %lu\n", effectiveDivisor);
	}
}

void withPrescaler(uint32_t targetDivisor) {
	uint32_t sysclkDivisor = targetDivisor;
	uint8_t sysclkDiv1 = 1;
	uint32_t prescaler = 1;
	
	if (sysclkDivisor > COUNTER_MAX) {
		prescaler = sysclkDivisor / COUNTER_MAX;
		
		if (prescaler > PRESCALER_MAX) {
			sysclkDiv1 = 0;
			prescaler /= 12;
			sysclkDivisor /= 12;
		}
		
		// Round the result
		if (sysclkDivisor % prescaler) {
			prescaler++;
		}
		
		sysclkDivisor /= prescaler;
	}
	
	printf("*** WITH prescaler:\n");
	
	uint32_t effectiveDivisor = sysclkDivisor * (sysclkDiv1 ? 1ul : 12ul) * prescaler;
	printf("Divide system clock by: %d\n", (sysclkDiv1 ? 1 : 12));
	printf("Prescaler divisor = %lu\n", prescaler);
	printf("Effective divisor = %lu\n", effectiveDivisor);
}

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("Usage: %s <system clock frequency in Hz> <target timer frequency in Hz>\n", argv[0]);
	} else {
		uint32_t systemClockFreq = strtoul(argv[1], NULL, 10);
		uint32_t targetTimerFreq = strtoul(argv[2], NULL, 10);
		uint32_t targetDivisor = systemClockFreq / targetTimerFreq;

		printf("System clock frequency = %lu Hz\n", systemClockFreq);
		printf("Target timer frequency = %lu Hz\n", targetTimerFreq);
		printf("Target divisor = %lu\n", targetDivisor);

		if (targetDivisor == 0) {
			printf("Target frequency is TOO HIGH.\n");
		} else {
			withoutPrescaler(targetDivisor);
			withPrescaler(targetDivisor);
		}
	}
}
