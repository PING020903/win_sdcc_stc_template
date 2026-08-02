#include "DBG_macro.h"
#include "tickBroadcast.h"
#include "userTask_cmds.h"

static EventSchedul_TaskNode *taskTable[TICKBROADCAST_MAX_TASKS];
static uint8_t taskCount = 0;

int tickBroadcast_Register(EventSchedul_TaskNode *task) {
    if (taskCount >= TICKBROADCAST_MAX_TASKS)
        return -1;
    if (task == NULL)
        return -2;
    taskTable[taskCount++] = task;
    return 0;
}

__HIGH_CODE
void tickBroadcast_Tick(void) {
    uint8_t i;
    for (i = 0; i < taskCount; i++) {
        EventSchedul_setEventToTask(evtSchedul_ctx, taskTable[i], EVT_TICK);
    }
}
