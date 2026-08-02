#ifndef _EVENTSCHEDUL_H_
#define _EVENTSCHEDUL_H_

#include <stddef.h>
#include <stdint.h>
#include "DBG_macro.h"

/* ==================== Scheduler context (opaque, static storage) ==================== */
typedef struct EventSchedul_Context EventSchedul_Context;

/* STC12 build: tasks live in a fixed pool, no dynamic allocation.
 * Pool sized for the single consolidated business task (console command
 * handling runs inside it) — XRAM only has 1 KB. Raise when adding tasks. */
#define EVTSCHEDUL_TASKS_MAX 2
#define EVTSCHEDUL_TASKS_QUEUE_MAX (EVTSCHEDUL_TASKS_MAX * 2)

typedef enum {
    EVTSCHEDUL_OK         = 0,
    EVTSCHEDUL_ERR_FAIL,
    EVTSCHEDUL_ERR_ARG,
    EVTSCHEDUL_ERR_MEM,
    EVTSCHEDUL_ERR_NOTHING,
    EVTSCHEDUL_ERR_EVENT,
    EVTSCHEDUL_ERR_TASK,
} EventSchedul_ErrCode;

typedef unsigned short EventSchedul_EventId;
typedef short          EventSchedul_TaskId;
typedef unsigned short EventSchedul_ExecCount;

#define EVTSCHEDUL_INVALID_EVT    ((EventSchedul_EventId)(0x00u-0x01u))
#define EVTSCHEDUL_INIT_EVT       ((EventSchedul_EventId)0x0000U)
#define EVTSCHEDUL_INIT_TASK_ID   ((EventSchedul_TaskId)0)
#define EVTSCHEDUL_INVALID_TASK_ID ((EventSchedul_TaskId)-1)

typedef struct {
    EventSchedul_EventId  eventStart;
    EventSchedul_EventId  eventEnd;
    EventSchedul_EventId  eventTrigger;
    EventSchedul_TaskId   taskId;
    EventSchedul_ExecCount executeCount;
} EventSchedul_TaskNodeInfo;

typedef void (*EventSchedul_pTaskFunc)(EventSchedul_EventId RecvEvt, void* arg) REENTRANT;

typedef struct EventSchedul_TaskNode {
    EventSchedul_TaskNodeInfo info;
    EventSchedul_pTaskFunc pTaskFunc;
    void* pTaskFuncArg;
} EventSchedul_TaskNode;

typedef struct {
    EventSchedul_TaskNode* taskHandle;
    EventSchedul_EventId   eventTrigger;
} EventSchedul_TaskQueue;

/* Kept for API compatibility; the static build ignores the allocator. */
typedef void *(*EventSchedul_malloc_fn)(size_t size);
typedef void (*EventSchedul_free_fn)(void *ptr);
typedef struct {
    EventSchedul_malloc_fn malloc;
    EventSchedul_free_fn   free;
} EventSchedul_Allocator;

/* ==================== Lifecycle ==================== */
EventSchedul_Context* EventSchedul_Create(const EventSchedul_Allocator* allocator);
void EventSchedul_Destroy(EventSchedul_Context* ctx);

/* ==================== Task management ==================== */
EventSchedul_TaskNode* EventSchedul_TaskRegister(EventSchedul_Context* ctx, const EventSchedul_TaskNode* cfg);
int EventSchedul_TaskUnRegister(EventSchedul_Context* ctx, EventSchedul_TaskNode* taskHandle);
int EventSchedul_setEventToTask(EventSchedul_Context* ctx, const EventSchedul_TaskNode* task,
    EventSchedul_EventId TaskEvent);

int EventSchedul_RegSleepMethod(EventSchedul_Context* ctx, void (*pFunc)(void) REENTRANT);
int EventSchedul_MainLoop(EventSchedul_Context* ctx);
int EventSchedul_TmosPoll(EventSchedul_Context* ctx);

#endif /* _EVENTSCHEDUL_H_ */
