#include "btnEventMap.h"
#include "userTask_doorLock.h"

/* Door buttons 0..3 open their door on the RELEASE edge (a full
 * press-then-release commits one open request); config keys 4..7 act on
 * PRESS. Change a trigger here to invert the behaviour of one key. */
static const btnEventMap_t btnEvtMap[] = {
    { BTN_CH_DOOR_0, BTN_TRIG_RELEASE, EVT_DOOR_OPEN_0 },
    { BTN_CH_DOOR_1, BTN_TRIG_RELEASE, EVT_DOOR_OPEN_1 },
    { BTN_CH_DOOR_2, BTN_TRIG_RELEASE, EVT_DOOR_OPEN_2 },
    { BTN_CH_DOOR_3, BTN_TRIG_RELEASE, EVT_DOOR_OPEN_3 },

    { BTN_CH_CFG_INC,    BTN_TRIG_PRESS, EVT_CFG_INC },
    { BTN_CH_CFG_DEC,    BTN_TRIG_PRESS, EVT_CFG_DEC },
    { BTN_CH_CFG_SELECT, BTN_TRIG_PRESS, EVT_CFG_SELECT },
    { BTN_CH_CFG_ENTER,  BTN_TRIG_PRESS, EVT_CFG_ENTER },
};

#define BTN_EVT_MAP_SIZE (sizeof(btnEvtMap) / sizeof(btnEvtMap[0]))

EventSchedul_EventId btnEventMap_resolve(uint8_t channel, uint8_t trigger)
{
    uint8_t i;

    for (i = 0; i < BTN_EVT_MAP_SIZE; i++) {
        if (btnEvtMap[i].channel == channel && btnEvtMap[i].trigger == trigger)
            return btnEvtMap[i].eventId;
    }
    return EVTSCHEDUL_INVALID_EVT;
}
