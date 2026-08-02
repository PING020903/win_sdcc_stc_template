#ifndef _STC15W10X_DIP8_H
#define _STC15W10X_DIP8_H

/**
 * This MCU needs the following additional command-line options:
 * 
 * - SDCC: --iram-size 128 --xram-size 0
 * 
 * - STCGAL: -b 38400
 */

#define MCU_FAMILY 15
#define MCU_SERIES 'W'
#define MCU_PINS 8
#define MCU_MAX_FREQ_MHZ 35
#define MCU_HAS_NO_SPI
#define MCU_HAS_NO_P5
#define NB_TIMERS 2
#define NB_UARTS 0
#define PCA_CHANNELS 0
#define ADC_CHANNELS 0

#include <uni-STC/stcmcu.h>

#endif // _STC15W10X_DIP8_H
