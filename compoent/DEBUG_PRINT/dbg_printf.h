#ifndef _DBG_PRINTF_H_
#define _DBG_PRINTF_H_

/*
 * dbg_printf.h — minimal variadic formatter for STC12 (SDCC / 8051).
 *
 * Replacement for the DEBUG_PRINT family's use of SDCC's printf. A variadic
 * function on mcs51 must be __reentrant, so every argument is pushed on the
 * IRAM stack; this formatter keeps that footprint as small as possible (no
 * width/flags/precision/long/float support, no strlen, direct UART output).
 */

#ifdef __SDCC
#define DBG_PRINTF_REENTRANT __reentrant
#else
#define DBG_PRINTF_REENTRANT
#endif

/* Supported conversion specifiers: %s %c %u %d %x %p %%.
 * Anything else prints '%' followed by the offending character. */
void dbg_printf(const char *fmt, ...) DBG_PRINTF_REENTRANT;

#endif /* _DBG_PRINTF_H_ */
