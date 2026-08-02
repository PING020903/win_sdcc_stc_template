#include "bus_io_management.h"
#include "DBG_macro.h"

#ifdef BUSMANAGE_DEBUG
#include <stdio.h>
#endif

/* Trimmed runtime state: just the active model and an initialised flag. */
static struct {
    const busManage_bus_model_t *current_model;
    unsigned short initialized;
} g_mgr = {0};

busManage_alloc_result_t busManage_manager_init(const busManage_bus_model_t *model)
{
    unsigned short i;

    if (!model || !model->resources || model->resourcesCnt == 0)
        return BUS_ALLOC_INVALID_ARG;

    for (i = 0; i < model->resourcesCnt; i++) {
        switch (model->resources[i].type) {
        case mcu_busType_UART:
        case mcu_busType_custom:
            break;
        default:
            return BUS_ALLOC_NOT_AVAILABLE;
        }
    }

    g_mgr.current_model = model;
    g_mgr.initialized = 1;
    return BUS_ALLOC_OK;
}

busManage_alloc_result_t busManage_manager_allocate(const busManage_resource_desc_t *bus)
{
    const busManage_resource_desc_t *begin;
    const busManage_resource_desc_t *end;

    if (!bus || !g_mgr.initialized)
        return BUS_ALLOC_INVALID_ARG;

    /* Trimmed check: the resource must belong to the active model. */
    begin = &g_mgr.current_model->resources[0];
    end   = &g_mgr.current_model->resources[g_mgr.current_model->resourcesCnt];
    if (bus < begin || bus >= end)
        return BUS_ALLOC_NOT_AVAILABLE;

    if (bus->init) {
        if (bus->init(bus) != 0)
            return BUS_ALLOC_INIT_FAIL;
    }
    return BUS_ALLOC_OK;
}

busManage_alloc_result_t busManage_manager_release(const busManage_resource_desc_t *bus)
{
    (void)bus;
    return BUS_ALLOC_OK;   /* no runtime state to release in the trimmed manager */
}

#ifdef BUSMANAGE_DEBUG
static const char *bus_type_str(busManage_bus_type_t t)
{
    switch (t) {
    case mcu_busType_UART:   return "UART";
    case mcu_busType_custom: return "CUSTOM";
    default:                 return "UNKNOWN";
    }
}

static const char *res_name(const busManage_resource_desc_t *r)
{
    if (r->type == mcu_busType_custom && r->desc.custom.descString)
        return r->desc.custom.descString;
    if (r->type == mcu_busType_UART && r->desc.uart.descString)
        return r->desc.uart.descString;
    return "N/A";
}

void busManage_manager_dump_status(void)
{
    unsigned short i;
    const busManage_bus_model_t *m = g_mgr.current_model;

    printf("[busmgr] init=%u model=%s\n",
           g_mgr.initialized, (m && m->device_name) ? m->device_name : "N/A");
    if (!m)
        return;
    for (i = 0; i < m->resourcesCnt; i++) {
        const busManage_resource_desc_t *r = &m->resources[i];
        printf("[busmgr] [%u] %s %s\n", i, bus_type_str(r->type), res_name(r));
    }
}
#endif
