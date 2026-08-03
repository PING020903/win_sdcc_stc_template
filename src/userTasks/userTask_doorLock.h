#ifndef _USERTASK_DOORLOCK_H_
#define _USERTASK_DOORLOCK_H_

#include "EventSchedul.h"
#include "doorLock.h"
#include "userButton.h"

typedef enum {
    EVT_DOORLOCK_INIT = 0x0000U,
    EVT_DOORLOCK_TICK,
    EVT_DOORLOCK_DETECT,
    EVT_DOOR_OPEN_0,
    EVT_DOOR_OPEN_1,
    EVT_DOOR_OPEN_2,
    EVT_DOOR_OPEN_3,
    EVT_CFG_INC,
    EVT_CFG_DEC,
    EVT_CFG_SELECT,
    EVT_CFG_ENTER,
    EVT_DOORLOCK_LOCK_TIMEOUT,
    EVT_DOORLOCK_BTN_SCAN,
    EVT_DOORLOCK_DISPLAY_REFRESH,
    EVT_DOORLOCK_TEST_SEC,      /* TEMP: 1 s heartbeat while board IO is undefined */
    EVT_DOORLOCK_END,
} doorLock_event_t;

/* Unified button channels (0..BUTTON_CNT-1, see userButton.h):
 *   BTN_CH_DOOR_n  per-door buttons (release -> open that door)
 *   BTN_CH_CFG_*   global config keys (+ - select enter) */
#define BTN_CH_DOOR_0   0
#define BTN_CH_DOOR_1   1
#define BTN_CH_DOOR_2   2
#define BTN_CH_DOOR_3   3
#define BTN_CH_CFG_INC   4   /* + : increase lock delay */
#define BTN_CH_CFG_DEC   5   /* - : decrease lock delay */
#define BTN_CH_CFG_SELECT 6  /* select door number */
#define BTN_CH_CFG_ENTER 7   /* enter config mode for the selected door */

int doorLockTask_init(void);

/* Board glue entry points (called from board_bus.c polling). */
void doorLockTask_requestDetect(unsigned char doorMask);
void doorLockTask_onButton(unsigned char channel, unsigned char level);
doorLock_manager_t *doorLockTask_manager(void);

#endif /* _USERTASK_DOORLOCK_H_ */
