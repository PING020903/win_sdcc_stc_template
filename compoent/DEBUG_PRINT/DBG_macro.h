#ifndef _DBG_MACRO_H_
#define _DBG_MACRO_H_

/*
 * Lightweight DBG_macro for STC12 (SDCC / 8051).
 *
 * Diagnostics are routed through the minimal low-stack dbg_printf() (see
 * dbg_printf.c), which writes straight to the UART1 FIFO in userUART_init.c.
 * All formatting code lives in flash; no large RAM buffer is used (the CH58x
 * version needed a 256-byte string buffer that does not fit in 1 KB XRAM).
 *
 * Set DBG_ENABLE to 0 to compile every diagnostic out entirely.
 */

#include <stdint.h>
#include <stddef.h>

/* CH58x placed hot code in a dedicated section; meaningless on 8051. */
#ifndef __HIGH_CODE
#define __HIGH_CODE
#endif

#ifndef __INTERRUPT
#define __INTERRUPT
#endif

/* 8051: functions invoked through a pointer must be reentrant. */
#ifdef __SDCC
#define REENTRANT __reentrant
#else
#define REENTRANT
#endif

#define SET_BIT(REG, BIT)         ((REG) |= (1U << (BIT)))
#define CLEAR_BIT(REG, BIT)       ((REG) &= ~(1U << (BIT)))
#define CHECK_BIT(REG, BIT)       ((REG) & (1U << (BIT)))

#define SIZE_ARRARY(ARR)          (sizeof(ARR) / sizeof((ARR)[0]))
#define VAR_NAME(x)               #x

#ifndef DBG_ENABLE
#define DBG_ENABLE 1
#endif

#if DBG_ENABLE

#include "dbg_printf.h"

/* The "[__func__] " prefix goes through dbg_prefix() (non-reentrant, arg
 * via PARM area) instead of a "%s" vararg, saving 3 B of stack at every
 * call site while keeping the output identical. */

#define DEBUG_PRINT(FMT, ...)     do { dbg_prefix(__func__); dbg_printf(FMT "\n", ##__VA_ARGS__); } while (0)
#define ERROR_PRINT(FMT, ...)     do { dbg_prefix(__func__); dbg_printf("ERR " FMT "\n", ##__VA_ARGS__); } while (0)

#define VAR_PRINT_UD(VAR)         do { dbg_prefix(__func__); dbg_printf("%s=%u (L%d)\n",  #VAR, (unsigned int)(VAR), __LINE__); } while (0)
#define VAR_PRINT_INT(VAR)        do { dbg_prefix(__func__); dbg_printf("%s=%d (L%d)\n",  #VAR, (int)(VAR), __LINE__); } while (0)
#define VAR_PRINT_HEX(VAR)        do { dbg_prefix(__func__); dbg_printf("%s=0x%x (L%d)\n", #VAR, (unsigned int)(VAR), __LINE__); } while (0)
#define VAR_PRINT_POS(VAR)        do { dbg_prefix(__func__); dbg_printf("%s=%p (L%d)\n",  #VAR, (void *)(VAR), __LINE__); } while (0)
#define VAR_PRINT_STRING(VAR)     do { dbg_prefix(__func__); dbg_printf("%s=\"%s\" (L%d)\n", #VAR, (VAR), __LINE__); } while (0)
#define VAR_PRINT_CH(VAR)         do { dbg_prefix(__func__); dbg_printf("%s='%c' (L%d)\n", #VAR, (char)(VAR), __LINE__); } while (0)

#define MACRO_PRINT_INT(VAR)      do { dbg_prefix(__func__); dbg_printf("MACRO %s=%d\n",  #VAR, (int)(VAR)); } while (0)
#define MACRO_PRINT_UD(VAR)       do { dbg_prefix(__func__); dbg_printf("MACRO %s=%u\n",  #VAR, (unsigned int)(VAR)); } while (0)
#define MACRO_PRINT_HEX(VAR)      do { dbg_prefix(__func__); dbg_printf("MACRO %s=0x%x\n", #VAR, (unsigned int)(VAR)); } while (0)
#define MACRO_PRINT_STR(VAR)      do { dbg_prefix(__func__); dbg_printf("MACRO %s=\"%s\"\n", #VAR, (VAR)); } while (0)

#else /* !DBG_ENABLE */

#define DEBUG_PRINT(FMT, ...)     do {} while (0)
#define ERROR_PRINT(FMT, ...)     do {} while (0)
#define VAR_PRINT_UD(VAR)         do {} while (0)
#define VAR_PRINT_INT(VAR)        do {} while (0)
#define VAR_PRINT_HEX(VAR)        do {} while (0)
#define VAR_PRINT_POS(VAR)        do {} while (0)
#define VAR_PRINT_STRING(VAR)     do {} while (0)
#define VAR_PRINT_CH(VAR)         do {} while (0)
#define MACRO_PRINT_INT(VAR)      do {} while (0)
#define MACRO_PRINT_UD(VAR)       do {} while (0)
#define MACRO_PRINT_HEX(VAR)      do {} while (0)
#define MACRO_PRINT_STR(VAR)      do {} while (0)

#endif /* DBG_ENABLE */

/* 64-bit / float / array dumps are kept silent to avoid pulling large
 * printf support code onto the 8051. */
#define VAR_PRINT_LLU(VAR)        do {} while (0)
#define VAR_PRINT_LL(VAR)         do {} while (0)
#define VAR_PRINT_FLOAT(VAR)      do {} while (0)
#define VAR_PRINT_ARR_HEX(VAR, _VAR_SIZE) do {} while (0)
#define DEBUG_PRINT_FUNC(FUNCTION, RET, RET_FORMAT) do { RET = FUNCTION; } while (0)

#endif /* _DBG_MACRO_H_ */
