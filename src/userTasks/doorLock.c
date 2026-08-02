#include "doorLock.h"

void doorLock_managerInit(doorLock_manager_t *mgr)
{
    mgr->doorCnt = 0;
    mgr->openDoor = -1;
}

doorLock_err_t doorLock_register(doorLock_manager_t *mgr, const doorLock_hwConfig_t *cfg, const busManage_resource_desc_t *res)
{
    if (mgr->doorCnt >= DOORLOCK_DOOR_MAX)
        return doorLock_err_full;

    (void)res;

    doorLock_context_t *ctx = &mgr->doors[mgr->doorCnt];
    ctx->hw = *cfg;
    ctx->flags = 0;
    ctx->index = mgr->doorCnt;
    doorLock_setInit(ctx);

    if (ctx->hw.ops && ctx->hw.ops->doorInit)
        ctx->hw.ops->doorInit(ctx);

    mgr->doorCnt++;
    return doorLock_err_none;
}

doorLock_err_t doorLock_detect(doorLock_context_t *ctx)
{
    if (!doorLock_isInit(ctx))
        return doorLock_err_not_init;

    if (!ctx->hw.ops || !ctx->hw.ops->detect)
        return doorLock_err_not_init;

    return ctx->hw.ops->detect(ctx);
}

doorLock_err_t doorLock_lockCtrl(doorLock_context_t *ctx, unsigned char lock)
{
    if (!doorLock_isInit(ctx))
        return doorLock_err_not_init;

    if (!ctx->hw.ops || !ctx->hw.ops->lock)
        return doorLock_err_not_init;

    return ctx->hw.ops->lock(ctx, lock);
}

doorLock_err_t doorLock_requestOpen(doorLock_manager_t *mgr, unsigned char doorIdx)
{
    if (doorIdx >= mgr->doorCnt)
        return doorLock_err_io_invalid;

    doorLock_context_t *target = &mgr->doors[doorIdx];
    if (!doorLock_isInit(target))
        return doorLock_err_not_init;

    if (mgr->openDoor >= 0 && mgr->openDoor != (signed char)doorIdx)
        doorLock_lockCtrl(&mgr->doors[mgr->openDoor], 1);

    doorLock_err_t err = doorLock_lockCtrl(target, 0);
    if (err != doorLock_err_none)
        return err;

    mgr->openDoor = (signed char)doorIdx;
    return doorLock_err_none;
}
