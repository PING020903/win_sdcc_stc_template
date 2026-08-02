#include "DBG_macro.h"
#include <string.h>
#include "EventSchedul.h"
#include "ringBuffer.h"

/* ==================== Scheduler context (single static instance) ==================== */
struct EventSchedul_Context {
    EventSchedul_TaskNode taskPool[EVTSCHEDUL_TASKS_MAX];
    void (*sleepMethod)(void) REENTRANT;
    EventSchedul_TaskId   taskNum;
    uint8_t  ringBuf_buffer[EVTSCHEDUL_TASKS_QUEUE_MAX * sizeof(EventSchedul_TaskQueue)];
    ringbuf_t ringTaskQueue;
};

static EventSchedul_Context _evtCtx;

/* ==================== Lifecycle ==================== */

__HIGH_CODE
EventSchedul_Context* EventSchedul_Create(const EventSchedul_Allocator* allocator)
{
    EventSchedul_Context* ctx = &_evtCtx;
    (void)allocator;

    ctx->sleepMethod = NULL;
    ctx->taskNum     = EVTSCHEDUL_INIT_TASK_ID;

    memset(ctx->taskPool, 0, sizeof(ctx->taskPool));

    {
        ringbuf_t tmp = RINGBUFCRTL_INIT(ctx->ringBuf_buffer,
                                          EVTSCHEDUL_TASKS_QUEUE_MAX,
                                          sizeof(EventSchedul_TaskQueue), false);
        memcpy(&ctx->ringTaskQueue, &tmp, sizeof(ringbuf_t));
    }
    return ctx;
}

__HIGH_CODE
void EventSchedul_Destroy(EventSchedul_Context* ctx)
{
    (void)ctx;
}

/* ==================== Internal helpers ==================== */

__HIGH_CODE
static inline int pushTaskToQueue(EventSchedul_Context* ctx, const EventSchedul_TaskQueue* task)
{
    ringBuf_err_t err;
    if (!task)
        return EVTSCHEDUL_ERR_ARG;

    err = ringBuf_push(&ctx->ringTaskQueue, task);
    if (err == RINGBUF_ERR_WR_DENIED)
        return EVTSCHEDUL_ERR_MEM;
    if (err != RINGBUF_OK)
        return EVTSCHEDUL_ERR_FAIL;

    return EVTSCHEDUL_OK;
}

__HIGH_CODE
static inline int pullTaskWithQueue(EventSchedul_Context* ctx, EventSchedul_TaskNode** task)
{
    EventSchedul_TaskQueue queueItem;
    ringBuf_err_t err;

    if (!task)
        return EVTSCHEDUL_ERR_ARG;

    err = ringBuf_pop(&ctx->ringTaskQueue, &queueItem);
    if (err == RINGBUF_ERR_EMPTY)
        return EVTSCHEDUL_ERR_NOTHING;
    if (err != RINGBUF_OK)
        return EVTSCHEDUL_ERR_FAIL;

    *task = queueItem.taskHandle;
    (*task)->info.eventTrigger = queueItem.eventTrigger;

    return EVTSCHEDUL_OK;
}

__HIGH_CODE
static EventSchedul_TaskNode* FindOrCreateFreeTaskNode(EventSchedul_Context* ctx)
{
    int i;
    for (i = 0; i < EVTSCHEDUL_TASKS_MAX; i++) {
        if (ctx->taskPool[i].pTaskFunc == NULL)
            return &ctx->taskPool[i];
    }
    return NULL;
}

/* ==================== Public API ==================== */

__HIGH_CODE
EventSchedul_TaskNode* EventSchedul_TaskRegister(EventSchedul_Context* ctx, const EventSchedul_TaskNode* cfg)
{
    EventSchedul_TaskNode* task = NULL;
    if (!ctx || !cfg || !cfg->pTaskFunc)
        return NULL;

    task = FindOrCreateFreeTaskNode(ctx);
    if (!task)
        return NULL;

    task->pTaskFunc       = cfg->pTaskFunc;
    task->pTaskFuncArg    = cfg->pTaskFuncArg;
    task->info.eventStart = cfg->info.eventStart;
    task->info.eventEnd   = cfg->info.eventEnd;
    task->info.taskId     = ++ctx->taskNum;

    if (ctx->taskNum < (EventSchedul_TaskId)0) {
        memset(task, 0, sizeof(EventSchedul_TaskNode));
        return NULL;
    }

    return task;
}

__HIGH_CODE
int EventSchedul_TaskUnRegister(EventSchedul_Context* ctx, EventSchedul_TaskNode* taskHandle)
{
    if (!ctx || !taskHandle)
        return EVTSCHEDUL_ERR_ARG;

    memset(taskHandle, 0, sizeof(EventSchedul_TaskNode));
    return EVTSCHEDUL_OK;
}

__HIGH_CODE
int EventSchedul_setEventToTask(EventSchedul_Context* ctx, const EventSchedul_TaskNode* target,
                                EventSchedul_EventId TaskEvent)
{
    int ret = EVTSCHEDUL_OK;
    const EventSchedul_TaskQueue tmp = {
        .taskHandle   = (EventSchedul_TaskNode*)target,
        .eventTrigger = TaskEvent
    };
    if (!ctx || !target)
        return EVTSCHEDUL_ERR_ARG;

    if (target < &ctx->taskPool[0] || target >= &ctx->taskPool[EVTSCHEDUL_TASKS_MAX]
        || target->pTaskFunc == NULL)
        return EVTSCHEDUL_ERR_TASK;

    switch (TaskEvent) {
    case EVTSCHEDUL_INVALID_EVT:
    case EVTSCHEDUL_INIT_EVT:
        return EVTSCHEDUL_ERR_ARG;
    default:
        ret = (target->info.eventStart > target->info.eventEnd) ? 1 : 0;
        if (ret) {
            if (TaskEvent < target->info.eventStart && TaskEvent >= target->info.eventEnd)
                return EVTSCHEDUL_ERR_EVENT;
        } else {
            if (TaskEvent < target->info.eventStart || TaskEvent >= target->info.eventEnd)
                return EVTSCHEDUL_ERR_EVENT;
        }
        break;
    }

    ret = pushTaskToQueue(ctx, &tmp);
    return ret;
}

__HIGH_CODE
int EventSchedul_RegSleepMethod(EventSchedul_Context* ctx, void (*pFunc)(void) REENTRANT)
{
    if (!ctx || !pFunc)
        return EVTSCHEDUL_ERR_ARG;

    ctx->sleepMethod = pFunc;
    return EVTSCHEDUL_OK;
}

__HIGH_CODE
int EventSchedul_MainLoop(EventSchedul_Context* ctx)
{
    int ret = EVTSCHEDUL_OK;
    EventSchedul_TaskNode* current = NULL;

    if (!ctx)
        return EVTSCHEDUL_ERR_ARG;

    ringBuf_init(&ctx->ringTaskQueue);

    if (!ctx->sleepMethod)
        goto _end;

    while (1) {
        current = NULL;
        ret = pullTaskWithQueue(ctx, &current);
        ctx->sleepMethod();
        if (ret != EVTSCHEDUL_OK || current == NULL)
            continue;

        current->pTaskFunc(current->info.eventTrigger, current->pTaskFuncArg);
        ctx->sleepMethod();
    }

_end:
    return EVTSCHEDUL_ERR_FAIL;
}

int EventSchedul_TmosPoll(EventSchedul_Context* ctx)
{
    int ret;
    EventSchedul_TaskNode* current = NULL;

    if (!ctx)
        return EVTSCHEDUL_ERR_ARG;

    ret = pullTaskWithQueue(ctx, &current);
    if (ret != EVTSCHEDUL_OK || current == NULL)
        return ret;

    current->pTaskFunc(current->info.eventTrigger, current->pTaskFuncArg);
    return EVTSCHEDUL_OK;
}
