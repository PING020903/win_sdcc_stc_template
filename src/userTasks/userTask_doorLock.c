#include "DBG_macro.h"
#include "userTask_doorLock.h"
#include "board_bus.h"
#include "tickBroadcast.h"
#include "userTask_cmds.h"
#include "userTMR_init.h"

static EventSchedul_TaskNode *hTaskDoorLock = NULL;
static doorLock_manager_t doorMgr;
static button_context_t btnCtx;
static button_rawEvt_t btnEvtBuf[BUTTON_EVT_QUEUE_DEPTH];

static volatile unsigned char detectPending = 0;
static volatile unsigned char doorBtnPending = 0;

static uint8_t lockTimeoutCnt = 0;     /* seconds */
static unsigned char lockTimeoutActive = 0;
static unsigned char selectedDoor = 0;

/* Config-mode state for the 4 global config keys. */
static unsigned char configMode = 0;
static uint8_t configIdleCnt = 0;      /* seconds */

/* TEMP: heartbeat counter for EVT_DOORLOCK_TEST_SEC. */
static uint16_t testSecCnt = 0;

#define CFG_DELAY_STEP_SEC     1U   /* +/- step per press (seconds) */
#define CONFIG_IDLE_TIMEOUT_SEC 5U   /* no input -> confirm current value (seconds) */

__HIGH_CODE
static void doorLockTask_postEvent(EventSchedul_EventId evt) {
    EventSchedul_setEventToTask(evtSchedul_ctx, hTaskDoorLock, evt);
}

/* Called by the board glue when one or more door trigger lines fire. */
void doorLockTask_requestDetect(unsigned char doorMask) {
    detectPending |= doorMask;
    doorLockTask_postEvent(EVT_DOORLOCK_DETECT);
}

/* Called by the board glue when one or more per-door buttons are pressed. */
void doorLockTask_requestDoorButton(unsigned char doorMask) {
    doorBtnPending |= doorMask;
    doorLockTask_postEvent(EVT_DOORLOCK_DOOR_BTN);
}

/* Called by the board glue on a button level change. */
void doorLockTask_onButton(unsigned char idx, unsigned char level) {
    button_push(&btnCtx, idx, level);
    doorLockTask_postEvent(EVT_DOORLOCK_BTN_SCAN);
}

doorLock_manager_t *doorLockTask_manager(void) {
    return &doorMgr;
}

static void doorLockTask_handleDetect(void) {
    unsigned char pending = detectPending;
    unsigned char i;
    detectPending = 0;

    for (i = 0; i < doorMgr.doorCnt; i++) {
        if (!(pending & (1U << i)))
            continue;
        /* 门磁只更新门状态标志（wire）；开门由门按键触发。 */
        doorLock_detect(&doorMgr.doors[i]);
    }
}

static void doorLockTask_handleDoorButton(void) {
    unsigned char pending = doorBtnPending;
    unsigned char i;
    doorBtnPending = 0;

    for (i = 0; i < doorMgr.doorCnt; i++) {
        doorLock_context_t *ctx;
        if (!(pending & (1U << i)))
            continue;
        ctx = &doorMgr.doors[i];
        doorLock_requestOpen(&doorMgr, i);
        if (ctx->hw.time.lockDelaySec > 0) {
            lockTimeoutCnt = ctx->hw.time.lockDelaySec;
            lockTimeoutActive = 1;
        }
    }
}

static void doorLockTask_handleTick(void) {
    static uint16_t secPrescaler = 0;

    /* button hold/long-press timing runs every 1 ms tick */
    button_tick(&btnCtx);

    /* derive a 1-second beat from the 1 ms tick */
    if (++secPrescaler < 1000U)
        return;
    secPrescaler = 0;

    doorLockTask_postEvent(EVT_DOORLOCK_TEST_SEC);

    if (lockTimeoutActive && lockTimeoutCnt > 0) {
        if (--lockTimeoutCnt == 0) {
            lockTimeoutActive = 0;
            doorLockTask_postEvent(EVT_DOORLOCK_LOCK_TIMEOUT);
        }
    }

    if (configMode && configIdleCnt > 0) {
        if (--configIdleCnt == 0) {
            configMode = 0;   /* inactivity: confirm current value */
            doorLockTask_postEvent(EVT_DOORLOCK_DISPLAY_REFRESH);
        }
    }
}

static void doorLockTask_handleButton(void) {
    unsigned char idx;
    button_event_t evt;
    unsigned char dirty = 0;

    while ((evt = button_poll(&btnCtx, &idx)) != button_event_none) {
        if (evt != button_event_press)
            continue;

        if (!configMode) {
            switch (idx) {
            case CFG_BTN_SELECT:
                selectedDoor++;
                if (selectedDoor >= doorMgr.doorCnt)
                    selectedDoor = 0;
                dirty = 1;
                break;
            case CFG_BTN_ENTER:
                configMode = 1;
                configIdleCnt = CONFIG_IDLE_TIMEOUT_SEC;
                dirty = 1;
                break;
            default:
                break;
            }
        } else {
            /* config mode: any key press restarts the inactivity timer */
            configIdleCnt = CONFIG_IDLE_TIMEOUT_SEC;
            switch (idx) {
            case CFG_BTN_INC: {
                doorLock_context_t *ctx = &doorMgr.doors[selectedDoor];
                ctx->hw.time.lockDelaySec += CFG_DELAY_STEP_SEC;
                dirty = 1;
            } break;
            case CFG_BTN_DEC: {
                doorLock_context_t *ctx = &doorMgr.doors[selectedDoor];
                if (ctx->hw.time.lockDelaySec >= CFG_DELAY_STEP_SEC)
                    ctx->hw.time.lockDelaySec -= CFG_DELAY_STEP_SEC;
                dirty = 1;
            } break;
            case CFG_BTN_SELECT:
            case CFG_BTN_ENTER:
                configMode = 0;   /* confirm & exit config mode */
                dirty = 1;
                break;
            default:
                break;
            }
        }
    }

    if (dirty)
        doorLockTask_postEvent(EVT_DOORLOCK_DISPLAY_REFRESH);
}

/* TEMP: heartbeat while the board IO is not finalised. */
static void doorLockTask_handleTestSec(void) {
    testSecCnt++;
    /* raw tick included: on a stall it shows whether the 1 ms beat died */
    DEBUG_PRINT("hb cnt=%u tick=%u", (unsigned int)testSecCnt, (unsigned int)userTMR_GetTick());
}

static void doorLockTask_handleLockTimeout(void) {
    if (doorMgr.openDoor >= 0) {
        doorLock_lockCtrl(&doorMgr.doors[doorMgr.openDoor], 1);
        doorMgr.openDoor = -1;
    }
}

static void doorLockTask(EventSchedul_EventId evt, void *arg) REENTRANT {
    (void)arg;

    switch (evt) {
    case EVT_DOORLOCK_TICK:
        board_bus_poll();
#if 0 /* TEMP: command tree disabled for isolation testing */
        cmds_poll();
#endif
        doorLockTask_handleTick();
        doorLockTask_handleButton();
        break;
    case EVT_DOORLOCK_DETECT:
        doorLockTask_handleDetect();
        break;
    case EVT_DOORLOCK_DOOR_BTN:
        doorLockTask_handleDoorButton();
        break;
    case EVT_DOORLOCK_DISPLAY_REFRESH:
        break;
    case EVT_DOORLOCK_BTN_SCAN:
        doorLockTask_handleButton();
        break;
    case EVT_DOORLOCK_LOCK_TIMEOUT:
        doorLockTask_handleLockTimeout();
        break;
    case EVT_DOORLOCK_TEST_SEC:
        doorLockTask_handleTestSec();
        break;
    default:
        break;
    }
}

int doorLockTask_init(void) {
    doorLock_managerInit(&doorMgr);
    button_init(&btnCtx, 1000, btnEvtBuf, BUTTON_EVT_QUEUE_DEPTH);

    EventSchedul_TaskNode cfg;
    cfg.pTaskFunc = doorLockTask;
    cfg.pTaskFuncArg = NULL;
    cfg.info.eventStart = EVT_DOORLOCK_INIT + 1;
    cfg.info.eventEnd = EVT_DOORLOCK_END;

    hTaskDoorLock = EventSchedul_TaskRegister(evtSchedul_ctx, &cfg);
    if (!hTaskDoorLock)
        return -1;

    tickBroadcast_Register(hTaskDoorLock);
    return 0;
}
