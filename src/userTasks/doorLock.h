#ifndef _DOORLOCK_H_
#define _DOORLOCK_H_

#include "bus_io_management.h"
#include "DBG_macro.h"

#define DOORLOCK_DOOR_MAX  4

/* openDoor 是 4 位有符号位域（最大正数 7），门编号必须落在此范围内。 */
#if DOORLOCK_DOOR_MAX > 7
#error "DOORLOCK_DOOR_MAX must be <= 7 (openDoor is a 4-bit signed field)"
#endif

typedef struct doorLock_context doorLock_context_t;

#define DOORLOCK_BIT_WIRE        0
#define DOORLOCK_BIT_LOCK        1
#define DOORLOCK_BIT_INIT        2

#define doorLock_setWire(ctx)      SET_BIT((ctx)->flags, DOORLOCK_BIT_WIRE)
#define doorLock_clrWire(ctx)      CLEAR_BIT((ctx)->flags, DOORLOCK_BIT_WIRE)
#define doorLock_isWire(ctx)       CHECK_BIT((ctx)->flags, DOORLOCK_BIT_WIRE)

#define doorLock_setLock(ctx)      SET_BIT((ctx)->flags, DOORLOCK_BIT_LOCK)
#define doorLock_clrLock(ctx)      CLEAR_BIT((ctx)->flags, DOORLOCK_BIT_LOCK)
#define doorLock_isLock(ctx)       CHECK_BIT((ctx)->flags, DOORLOCK_BIT_LOCK)

#define doorLock_setInit(ctx)      SET_BIT((ctx)->flags, DOORLOCK_BIT_INIT)
#define doorLock_clrInit(ctx)      CLEAR_BIT((ctx)->flags, DOORLOCK_BIT_INIT)
#define doorLock_isInit(ctx)       CHECK_BIT((ctx)->flags, DOORLOCK_BIT_INIT)

typedef enum {
    doorLock_err_none = 0,
    doorLock_err_not_init,
    doorLock_err_io_invalid,
    doorLock_err_timeout,
    doorLock_err_full,
} doorLock_err_t;

/* detect 读取硬件并更新上下文的 WIRE 状态，故 ctx 非 const；
 * lock 只驱动输出、不修改上下文，保持 const。 */
typedef doorLock_err_t (*doorLock_detectFn)(doorLock_context_t *ctx) REENTRANT;
typedef doorLock_err_t (*doorLock_lockFn)(const doorLock_context_t *ctx, unsigned char lock) REENTRANT;
typedef doorLock_err_t (*doorLock_doorInitFn)(doorLock_context_t *ctx) REENTRANT;

typedef struct {
    busManage_io_t detect;      /* 门磁状态输入 */
    busManage_io_t doorButton;  /* 门按键输入（触发开门） */
    busManage_io_t lock;        /* 继电器输出 */
} doorLock_io_t;

/* 门锁延时，单位：秒（精度到秒即可，uint8_t 足够）。 */
typedef struct {
    uint8_t lockDelaySec;
} doorLock_time_t;

/* 方法表：所有门共用的回调（detect / lock / doorInit）。 */
typedef struct {
    doorLock_detectFn detect;
    doorLock_lockFn lock;
    doorLock_doorInitFn doorInit;
} doorLock_ops_t;

typedef struct {
    doorLock_io_t io;
    doorLock_time_t time;
    const doorLock_ops_t *ops;   /* 指向共享方法表 */
} doorLock_hwConfig_t;

struct doorLock_context {
    doorLock_hwConfig_t hw;
    unsigned char flags : 4;     /* WIRE/LOCK/INIT 仅用 3 位 */
    unsigned char index : 4;     /* 门编号 0~DOORLOCK_DOOR_MAX-1 */
};

/* doorCnt 0~4、openDoor -1(无)~3，各 4 位足够（有符号 4 位可至 7）。 */
typedef struct {
    doorLock_context_t doors[DOORLOCK_DOOR_MAX];
    unsigned char doorCnt : 4;
    signed char openDoor : 4;
} doorLock_manager_t;

void doorLock_managerInit(doorLock_manager_t *mgr);
doorLock_err_t doorLock_register(doorLock_manager_t *mgr, const doorLock_hwConfig_t *cfg, const busManage_resource_desc_t *res);
doorLock_err_t doorLock_detect(doorLock_context_t *ctx);
doorLock_err_t doorLock_lockCtrl(doorLock_context_t *ctx, unsigned char lock);
doorLock_err_t doorLock_requestOpen(doorLock_manager_t *mgr, unsigned char doorIdx);

#endif
