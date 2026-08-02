#include "project-defs.h"
#include "userUART_init.h"
#include "ringBuffer.h"
#include <stdio.h>
#include <string.h>

/*
 * UART1 (P3.0 RxD / P3.1 TxD), Timer1 baud-rate generator, interrupt
 * driven TX and RX.
 *
 * UART_CFG_OFFICIAL selects the configuration of STC's official UART1
 * demo: Timer1 stays in its default 12T mode at 9600 baud and AUXR is
 * left alone. When 0, Timer1 is switched to 1T (AUXR.T1x12) for 115200
 * baud. With an 11.0592 MHz crystal both yield reload value 0xFD.
 * NOTE: the baud math needs the *exact* frequency; MCU_FREQ is defined as
 * 11059200ul in the build configuration (11059000 would truncate the
 * reload to 0xFE and produce 172800 baud).
 *
 * The RX/TX FIFOs use the project's ringBuffer component; the ringBuf_*
 * functions involved are declared REENTRANT so the ISR and the main
 * context may use them concurrently. Keep the depth <= 128 so the 16-bit
 * ring indices stay effectively atomic (high byte always zero).
 * userUART_WriteByte() blocks while the TX FIFO is full, so it requires
 * interrupts to be enabled.
 */

#define UART_CFG_OFFICIAL  0

#if UART_CFG_OFFICIAL
#define USER_DEFAULT_BAUDRATE  9600UL
#define UART1_RELOAD  ((uint8_t)(-(MCU_FREQ / 12UL / 32UL / USER_DEFAULT_BAUDRATE)))
#else
#define USER_DEFAULT_BAUDRATE  115200UL
#define UART1_RELOAD  ((uint8_t)(256U - (MCU_FREQ / USER_DEFAULT_BAUDRATE / 32UL)))
#endif

/* 16 (not 32): XRAM budget after the CommandParse component moved in.
 * TX blocking in userUART_WriteByte absorbs the smaller FIFO. */
#define UART_BUF_DEPTH  16U

static uint8_t rxStorage[UART_BUF_DEPTH];
static uint8_t txStorage[UART_BUF_DEPTH];
static ringbuf_t rxFifo;
static ringbuf_t txFifo;
static volatile uint8_t txBusy = 0;

static void fifoInit(ringbuf_t *fifo, void *storage)
{
    ringbuf_t tmp = RINGBUFCRTL_INIT(storage, UART_BUF_DEPTH, sizeof(uint8_t), false);
    memcpy(fifo, &tmp, sizeof(ringbuf_t));
    ringBuf_init(fifo);
}

void userUART_init(void)
{
    fifoInit(&rxFifo, rxStorage);
    fifoInit(&txFifo, txStorage);

#if !UART_CFG_OFFICIAL
    AUXR |= M_T1x12;                        /* Timer1 -> 1T mode */
#endif
    TMOD = (TMOD & ~M_T1_MODE) | 0x20;      /* Timer1 -> 8-bit auto-reload */
    T1H = UART1_RELOAD;
    T1L = UART1_RELOAD;
    S1CON = M_SM1 | M_REN;                  /* UART mode 1, receiver enable */
    T1RUN = 1;                              /* start Timer1 */
    IE1 |= M_S1IE;                          /* enable UART1 interrupt */
}

void userUART_WriteByte(uint8_t c)
{
    while (ringBuf_push(&txFifo, &c) != RINGBUF_OK)
        ;                                   /* FIFO full: wait for the ISR */

    IE1 &= ~M_S1IE;
    if (!txBusy) {                          /* kick the transmitter */
        uint8_t first;
        txBusy = 1;
        if (ringBuf_pop(&txFifo, &first) == RINGBUF_OK)
            S1BUF = first;
        else
            txBusy = 0;
    }
    IE1 |= M_S1IE;
}

void userUART_WriteString(const char *s)
{
    while (*s)
        userUART_WriteByte((uint8_t)*s++);
}

/* Block until the TX FIFO is drained and the last byte fully shifted out
 * (txBusy is cleared by the ISR on the final TI). Requires interrupts. */
void userUART_FlushTx(void)
{
    ringbuf_cnt_t cnt;

    do {
        ringBuf_count(&txFifo, &cnt);
    } while (cnt > 0 || txBusy);
}

uint8_t userUART_Available(void)
{
    ringbuf_cnt_t cnt = 0;
    ringBuf_count(&rxFifo, &cnt);
    return (uint8_t)cnt;
}

int16_t userUART_ReadByte(void)
{
    uint8_t c;
    if (ringBuf_pop(&rxFifo, &c) != RINGBUF_OK)
        return -1;
    return (int16_t)c;
}

/* Retarget SDCC's printf to UART1 (used by DBG_macro.h). */
int putchar(int c)
{
    if (c == '\n')
        userUART_WriteByte((uint8_t)'\r');
    userUART_WriteByte((uint8_t)c);
    return c;
}

INTERRUPT(uart1_isr, UART1_INTERRUPT)
{
    if (S1CON & M_UART_RXIF) {
        uint8_t c = S1BUF;
        S1CON &= ~M_UART_RXIF;
        ringBuf_push(&rxFifo, &c);          /* dropped when full */
    }
    if (S1CON & M_UART_TXIF) {
        uint8_t next;
        S1CON &= ~M_UART_TXIF;
        if (ringBuf_pop(&txFifo, &next) == RINGBUF_OK)
            S1BUF = next;
        else
            txBusy = 0;
    }
}
