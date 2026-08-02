#ifndef _STC8H2KXXU_TSSOP20_H
#define _STC8H2KXXU_TSSOP20_H

/**
 * NOTE: On the STC8H2, the pin traditionally used for P55 
 * is used for ADC_VRef+.
 */

#define MCU_FAMILY 8
#define MCU_SERIES 'H'
#define MCU_PINS 20
#define MCU_MAX_FREQ_MHZ 45
#define CHIPID_IN_XDATA
#define MCU_HAS_COMPARATOR
#define COMPARATOR_4P2N
#define MCU_HAS_MDU
#define MCU_HAS_RTC
#define MCU_HAS_USB
#define GPIO_HAS_INT_WK
#define GPIO_NO_P12
#define GPIO_NO_P55
#define NB_UARTS 2
#define PWM_GROUPS 2
#define PWM_CHANNELS 4
#define ADC_CHANNELS 15
#define MCU_HAS_ADCEXCFG
#define SPI_HAS_HIGH_SPEED
#define NB_TIMERS 4
#define TIMER_HAS_T11

#include <uni-STC/stcmcu.h>

#endif // _STC8H2KXXU_TSSOP20_H
