#ifndef _USERTASK_DOORLOCK_H_
#define _USERTASK_DOORLOCK_H_

#include "EventSchedul.h"
#include "doorLock.h"
#include "userButton.h"

typedef enum {
    EVT_DOORLOCK_INIT = 0x0000U,
    EVT_DOORLOCK_TICK,
    EVT_DOORLOCK_DETECT,
    EVT_DOORLOCK_DOOR_BTN,
    EVT_DOORLOCK_LOCK_TIMEOUT,
    EVT_DOORLOCK_BTN_SCAN,
    EVT_DOORLOCK_DISPLAY_REFRESH,
    EVT_DOORLOCK_TEST_SEC,      /* TEMP: 1 s heartbeat while board IO is undefined */
    EVT_DOORLOCK_END,
} doorLock_event_t;

/* Global config button indices (the 4 config keys, not the per-door buttons). */
#define CFG_BTN_INC     0   /* + : increase lock delay */
#define CFG_BTN_DEC     1   /* - : decrease lock delay */
#define CFG_BTN_SELECT  2   /* select door number */
#define CFG_BTN_ENTER   3   /* enter config mode for the selected door */

int doorLockTask_init(void);

/* Board glue entry points (called from board_bus.c polling). */
void doorLockTask_requestDetect(unsigned char doorMask);
void doorLockTask_requestDoorButton(unsigned char doorMask);
void doorLockTask_onButton(unsigned char idx, unsigned char level);
doorLock_manager_t *doorLockTask_manager(void);

#endif
