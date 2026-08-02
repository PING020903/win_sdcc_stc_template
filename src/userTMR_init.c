#include "project-defs.h"
#include "userTMR_init.h"

/*
 * System tick on Timer0.
 *
 * The door lock math treats one EVT_TICK as one millisecond, so we drive
 * Timer0 in 16-bit mode (mode 1) with a manual reload to fire every 1 ms.
 * Timer0 is put in 1T mode (AUXR.T0x12) so the reload count is
 * MCU_FREQ / 1000, which fits comfortably in 16 bits.
 *
 * The exact reload tracks the MCU_FREQ macro, so changing the crystal only
 * requires updating MCU_FREQ in the build configuration.
 */

#define TICK_HZ     1000UL
#define T0_RELOAD   ((uint16_t)(65536UL - (MCU_FREQ / TICK_HZ)))

static volatile uint16_t tickMs = 0;

void userTMR_init(void)
{
    AUXR |= M_T0x12;                       /* Timer0 -> 1T mode */
    TMOD = (TMOD & ~M_T0_MODE) | 0x01;     /* Timer0 -> 16-bit mode */
    T0 = T0_RELOAD;
    T0IE = 1;                              /* enable Timer0 interrupt */
    T0RUN = 1;                             /* start Timer0 */
}

uint16_t userTMR_GetTick(void)
{
    uint16_t t;

    /* tickMs is incremented by timer0_isr; exclude the only writer so the
     * 16-bit read is atomic. A pending overflow simply fires right after. */
    T0IE = 0;
    t = tickMs;
    T0IE = 1;
    return t;
}

INTERRUPT(timer0_isr, TIMER0_INTERRUPT)
{
    T0 = T0_RELOAD;
    tickMs++;
}
