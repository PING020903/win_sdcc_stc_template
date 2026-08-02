#ifndef _TICKBROADCAST_H_
#define _TICKBROADCAST_H_

#include "EventSchedul.h"

#define TICKBROADCAST_MAX_TASKS  8
#define EVT_TICK  ((EventSchedul_EventId)1)

int tickBroadcast_Register(EventSchedul_TaskNode *task);
void tickBroadcast_Tick(void);

#endif /* _TICKBROADCAST_H_ */
