#ifndef _STC8H4KXXTL_LQFP32_H
#define _STC8H4KXXTL_LQFP32_H

/**
 * NOTE: On the STC8H4, the pin traditionally used for P55 
 * is used for ADC_VRef+.
 */

#define MCU_FAMILY 8
#define MCU_SERIES 'H'
#define MCU_PINS 32
#define MCU_MAX_FREQ_MHZ 45
#define CHIPID_IN_CODE
#define MCU_HAS_COMPARATOR
#define COMPARATOR_4P2N
#define MCU_HAS_MDU
#define MCU_HAS_DMA
#define MCU_HAS_TOUCHKEY
#define MCU_HAS_LED
#define MCU_HAS_LCM
#define MCU_HAS_RTC
#define MCU_HAS_USB
#define GPIO_HAS_INT_WK
#define GPIO_NO_P12
#define GPIO_NO_P55
#define NB_UARTS 4
#define PWM_GROUPS 2
#define PWM_CHANNELS 4
#define ADC_CHANNELS 15
#define MCU_HAS_ADCEXCFG
#define SPI_HAS_HIGH_SPEED
#define NB_TIMERS 5
#define TIMER_HAS_T3_T4_PIN_SWITCH

#include <uni-STC/stcmcu.h>

#endif // _STC8H4KXXTL_LQFP32_H
