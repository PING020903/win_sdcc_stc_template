#include "dbg_printf.h"
#include "userUART_init.h"
#include <stdarg.h>

/*
 * dbg_printf.c — minimal low-stack printf for STC12 (SDCC / 8051).
 *
 * Stack budget (__reentrant frame, all on the IRAM stack):
 *   fmt ptr(3) + va_list(3) + loop ptr(3) + digit buf(5) + temps(~2)
 *   + return addr(2)  =>  ~16-18 B, vs SDCC printf's ~68 B (analyzer override).
 *
 * Deliberately omitted: width/precision/flags, %ld/%f, strlen. Output goes
 * straight to the UART FIFO (bypassing putchar/printf) one byte at a time.
 */

/* Emit one char; map '\n' to CRLF like the printf retarget does. */
static void dbg_putc(uint8_t c)
{
    if (c == '\n')
        userUART_WriteByte((uint8_t)'\r');
    userUART_WriteByte(c);
}

void dbg_prefix(const char *fn)
{
    dbg_putc('[');
    while (*fn)
        dbg_putc((uint8_t)*fn++);
    dbg_putc(']');
    dbg_putc(' ');
}

static void dbg_u16_dec(unsigned int v)
{
    char d[5];
    uint8_t i = 0;

    do {
        d[i++] = (char)('0' + (v % 10u));
        v /= 10u;
    } while (v);
    while (i)
        dbg_putc((uint8_t)d[--i]);
}

static void dbg_u16_hex(unsigned int v)
{
    char d[4];
    uint8_t i = 0;

    do {
        uint8_t n = (uint8_t)(v & 0x0Fu);
        d[i++] = (char)((n < 10u) ? ('0' + n) : ('a' + n - 10u));
        v >>= 4u;
    } while (v);
    while (i)
        dbg_putc((uint8_t)d[--i]);
}

void dbg_printf(const char *fmt, ...) DBG_PRINTF_REENTRANT
{
    va_list ap;

    va_start(ap, fmt);
    for (; *fmt; fmt++) {
        char c = *fmt;

        if (c != '%') {
            dbg_putc((uint8_t)c);
            continue;
        }
        fmt++;
        switch (*fmt) {
        case '%':
            dbg_putc('%');
            break;
        case 'c':
            dbg_putc((uint8_t)va_arg(ap, int));
            break;
        case 's': {
            const char *s = va_arg(ap, const char *);

            if (!s)
                s = "(null)";
            while (*s)
                dbg_putc((uint8_t)*s++);
            break;
        }
        case 'd': {
            int v = va_arg(ap, int);

            if (v < 0) {
                dbg_putc('-');
                v = -v;
            }
            dbg_u16_dec((unsigned int)v);
            break;
        }
        case 'u':
            dbg_u16_dec(va_arg(ap, unsigned int));
            break;
        case 'x':
        case 'p':
            dbg_u16_hex((unsigned int)va_arg(ap, unsigned int));
            break;
        default:
            dbg_putc('%');
            if (*fmt)
                dbg_putc((uint8_t)*fmt);
            break;
        }
    }
    va_end(ap);
}
