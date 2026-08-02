#include "project-defs.h"
#include "userUART_init.h"
#include <stdio.h>

/*
 * Minimal UART1 (P3.0 RxD / P3.1 TxD) using Timer1 as baud-rate generator.
 *
 * Kept deliberately lean (no FIFO buffers) to fit the STC12's 1 KB XRAM.
 * Timer1 runs in 8-bit auto-reload mode (mode 2), 1T, so the reload value
 * is 256 - MCU_FREQ / baud / 32. With an 11.0592 MHz crystal this yields
 * an exact 115200 baud (reload = 0xFD).
 */

#define USER_DEFAULT_BAUDRATE  115200UL
#define UART1_RELOAD  ((uint8_t)(256U - (MCU_FREQ / USER_DEFAULT_BAUDRATE / 32UL)))

void userUART_init(void)
{
    AUXR |= M_T1x12;                        /* Timer1 -> 1T mode */
    TMOD = (TMOD & ~M_T1_MODE) | 0x20;      /* Timer1 -> 8-bit auto-reload */
    T1H = UART1_RELOAD;
    T1L = UART1_RELOAD;
    S1CON = M_SM1 | M_REN;                  /* UART mode 1, receiver enable */
    T1RUN = 1;                              /* start Timer1 */
}

void userUART_WriteByte(uint8_t c)
{
    S1BUF = c;
    while (!(S1CON & M_UART_TXIF))
        ;
    S1CON &= ~M_UART_TXIF;
}

void userUART_WriteString(const char *s)
{
    while (*s)
        userUART_WriteByte((uint8_t)*s++);
}

/* Retarget SDCC's printf to UART1 (used by DBG_macro.h). */
int putchar(int c)
{
    if (c == '\n')
        userUART_WriteByte((uint8_t)'\r');
    userUART_WriteByte((uint8_t)c);
    return c;
}
