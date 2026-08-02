#include "DBG_macro.h"
#include "EventSchedul.h"
#include "board_bus.h"
#include "project-defs.h"
#include "tickBroadcast.h"
#include "userTMR_init.h"
#include "userTask_cmds.h"
#include "userTask_doorLock.h"
#include "userUART_init.h"


/* Shared scheduler handle used by tickBroadcast and the door lock task. */
EventSchedul_Context *evtSchedul_ctx = NULL;

/*
 * SDCC emits the interrupt vector table only in the module containing
 * main(), and only for ISRs prototyped here *with* the __interrupt
 * attribute. A plain prototype produces no vector entry.
 */
#ifdef __SDCC
void timer0_isr(void) __interrupt(TIMER0_INTERRUPT); /* userTMR_init.c */
void uart1_isr(void) __interrupt(UART1_INTERRUPT);   /* userUART_init.c */
#else
void timer0_isr(void);
void uart1_isr(void);
#endif

/* Scheduler idle hook: only the 1 ms tick broadcast lives here; all
 * business polling (IO sampling etc.) runs inside task event handlers. */
__HIGH_CODE
static void evtSchedul_idle(void) REENTRANT
{
    static uint16_t lastTick = 0;
    uint16_t now = userTMR_GetTick();

    if (now != lastTick) {
        lastTick = now;
        tickBroadcast_Tick();
    }
}

int main(void)
{
    CLKDIV = 0; /* defensive: sysclk = crystal/1 regardless of ISP settings */
    EA = 1;

    userUART_init();
    userTMR_init();

    DEBUG_PRINT("build %s %s", __DATE__, __TIME__);

    if (board_bus_init() != 0) {
        ERROR_PRINT("board_bus_init failed");
        while (1)
            ;
    }

    evtSchedul_ctx = EventSchedul_Create(NULL);
    EventSchedul_RegSleepMethod(evtSchedul_ctx, evtSchedul_idle);

    doorLockTask_init();
    board_registerDoors(doorLockTask_manager());

    DEBUG_PRINT("doorLock ready, doors=%u", (unsigned int)doorLockTask_manager()->doorCnt);

    EventSchedul_MainLoop(evtSchedul_ctx);
    return 0;
}
