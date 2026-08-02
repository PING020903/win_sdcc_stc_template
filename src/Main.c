#include "project-defs.h"
#include "DBG_macro.h"
#include "board_bus.h"
#include "userUART_init.h"
#include "userTMR_init.h"
#include "EventSchedul.h"
#include "tickBroadcast.h"
#include "userTask_doorLock.h"
#include "userTask_cmds.h"

/* Shared scheduler handle used by tickBroadcast and the door lock task. */
EventSchedul_Context* evtSchedul_ctx = NULL;

/* Defined in userTMR_init.c; declared here so SDCC emits its vector. */
void timer0_isr(void);

__HIGH_CODE
static void evtSchedul_idle(void) REENTRANT
{
    static uint16_t lastTick = 0;
    uint16_t now = userTMR_GetTick();

    board_bus_poll();

    if (now != lastTick) {
        lastTick = now;
        tickBroadcast_Tick();
    }
}

int main(void)
{
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
