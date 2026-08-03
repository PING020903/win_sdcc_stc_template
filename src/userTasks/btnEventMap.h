#ifndef _BTNEVENTMAP_H_
#define _BTNEVENTMAP_H_

#include "EventSchedul.h"

/* Button-event mapping: translate a button channel + trigger into the
 * EventScheduling event bound to the door-lock task. Editing this table
 * changes which physical button/edge drives which business action without
 * touching the poll loop or the task handlers. */

typedef enum {
    BTN_TRIG_PRESS = 0,
    BTN_TRIG_RELEASE,
} btnTrigger_t;

typedef struct {
    uint8_t channel;                 /* unified button channel (0..BUTTON_CNT-1) */
    uint8_t trigger;                 /* btnTrigger_t */
    EventSchedul_EventId eventId;    /* event posted to the door-lock task */
} btnEventMap_t;

/* Linear lookup over the map; returns EVTSCHEDUL_INVALID_EVT when unmapped. */
EventSchedul_EventId btnEventMap_resolve(uint8_t channel, uint8_t trigger);

#endif /* _BTNEVENTMAP_H_ */
