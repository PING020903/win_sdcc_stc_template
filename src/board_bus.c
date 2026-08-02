#include "project-defs.h"
#include <gpio-hal.h>
#include "board_bus.h"
#include "bus_io_management.h"
#include "userTask_doorLock.h"
#include "userTMR_init.h"
#include "debounce.h"
#include "DBG_macro.h"

/*
 * Board bus wiring (placeholder pins, active-low inputs).
 *
 * Each door owns three IOs described in its io table:
 *   [0] 门磁状态  detect input  (door sensor)
 *   [1] 门按键    door-button input (press -> open that door)
 *   [2] 继电器    relay output  (1 = locked)
 * The bus framework walks every resource and runs its init callback to
 * configure those IOs. The io tables are the single source of truth for pin
 * assignment — door registration and runtime polling both read from them.
 *
 *   Door detect (门磁)   : P1.0 .. P1.3   (high-impedance / 浮空输入)
 *   Door button (门按键) : P0.0 .. P0.3   (high-impedance / 浮空输入)
 *   Relay (继电器)       : P2.0 .. P2.3   (push-pull, 1 = locked)
 *   Config buttons       : P3.2 .. P3.5   (high-impedance; +, -, select, enter;
 *                                           P3.0/P3.1 = UART1)
 *
 * Inputs are active-low: external pull-up holds the pin high (idle); a 0Ω
 * path (door in place) or a pressed button pulls it low.
 *
 * Detection is POLLED, not interrupt-driven, by design: the STC12C5A60S2 has
 * only two general external interrupts — INT0 (P3.2) and INT1 (P3.3) — and no
 * per-pin GPIO edge interrupts (that is an STC8/STC15 feature). With 4 door
 * sensors + 4 door buttons + 4 config keys, per-pin interrupts are impossible
 * on this part, so the door-lock task calls board_bus_poll() from its 1 ms
 * TICK event handler; the function rate-limits itself to one sample every
 * 10 ms and debounces inputs (3 stable samples = 30 ms) before emitting
 * an event on the debounced EDGE. Debouncing + edge triggering together prevent
 * contact bounce from producing duplicate events. Door-lock events are
 * human-paced, so polling is fully responsive here.
 *
 * Per-input event policy (on debounced edges):
 *   - 门磁   : both edges -> requestDetect (updates door status)
 *   - 门按键 : release edge (low->high) -> requestDoorButton (open the door);
 *              a full press-then-release commits one open event
 *   - 配置键 : both edges -> onButton(stable level) into the button framework
 *
 * Adjust the io tables below to match the final hardware.
 */

#define LOCK_DEFAULT_DELAY_SEC  3U

/* Input sampling period (ms). Debounce depth is DEBOUNCE_SAMPLES (debounce.h). */
#define BOARD_POLL_PERIOD_MS    10U

/* ---- forward declarations of resource init callbacks ---- */
static int door_busInit(const busManage_resource_desc_t *res) REENTRANT;
static int button_busInit(const busManage_resource_desc_t *res) REENTRANT;
static int debug_uart_busInit(const busManage_resource_desc_t *res) REENTRANT;

/* ---- IO tables: index 0 = 门磁, 1 = 门按键, 2 = 继电器 ---- */
static const busManage_io_t door0_io[] = {
    {.port = BUSIO_PORT_P1, .pin = 0},   /* 门磁 detect */
    {.port = BUSIO_PORT_P0, .pin = 0},   /* 门按键 door button */
    {.port = BUSIO_PORT_P2, .pin = 0},   /* 继电器 relay */
};
static const busManage_io_t door1_io[] = {
    {.port = BUSIO_PORT_P1, .pin = 1},
    {.port = BUSIO_PORT_P0, .pin = 1},
    {.port = BUSIO_PORT_P2, .pin = 1},
};
static const busManage_io_t door2_io[] = {
    {.port = BUSIO_PORT_P1, .pin = 2},
    {.port = BUSIO_PORT_P0, .pin = 2},
    {.port = BUSIO_PORT_P2, .pin = 2},
};
static const busManage_io_t door3_io[] = {
    {.port = BUSIO_PORT_P1, .pin = 3},
    {.port = BUSIO_PORT_P0, .pin = 3},
    {.port = BUSIO_PORT_P2, .pin = 3},
};

/* 4 global config buttons: +, -, select door, enter config. */
static const busManage_io_t button_io[] = {
    {.port = BUSIO_PORT_P3, .pin = 2},   /* CFG_BTN_INC */
    {.port = BUSIO_PORT_P3, .pin = 3},   /* CFG_BTN_DEC */
    {.port = BUSIO_PORT_P3, .pin = 4},   /* CFG_BTN_SELECT */
    {.port = BUSIO_PORT_P3, .pin = 5},   /* CFG_BTN_ENTER */
};

/* ---- resource table (one entry per resource) ---- */
/* SDCC cannot copy struct variables into an array initializer, so each
 * resource is described inline; doors occupy contiguous indices so the
 * door-lock registration can index them directly. */
enum {
    RES_DOOR0 = 0,
    RES_DOOR1,
    RES_DOOR2,
    RES_DOOR3,
    RES_BUTTON,
    RES_UART,
    RES_COUNT
};

static const busManage_resource_desc_t board_all_resources[] = {
    [RES_DOOR0] = {.type = mcu_busType_custom,
                   .desc.custom = {.io_count = 3, .io_table = door0_io, .descString = "Door0"},
                   .init = door_busInit},
    [RES_DOOR1] = {.type = mcu_busType_custom,
                   .desc.custom = {.io_count = 3, .io_table = door1_io, .descString = "Door1"},
                   .init = door_busInit},
    [RES_DOOR2] = {.type = mcu_busType_custom,
                   .desc.custom = {.io_count = 3, .io_table = door2_io, .descString = "Door2"},
                   .init = door_busInit},
    [RES_DOOR3] = {.type = mcu_busType_custom,
                   .desc.custom = {.io_count = 3, .io_table = door3_io, .descString = "Door3"},
                   .init = door_busInit},
    [RES_BUTTON] = {.type = mcu_busType_custom,
                    .desc.custom = {.io_count = BUTTON_CNT, .io_table = button_io, .descString = "Buttons"},
                    .init = button_busInit},
    [RES_UART] = {.type = mcu_busType_UART,
                  .desc.uart = {.uart_instance = 1, .baudrate = 115200UL, .descString = "Debug_UART1"},
                  .init = debug_uart_busInit},
};

/* ---- device model ---- */
static const busManage_bus_model_t board_busModel = {
    .device_name = "SimpleDoorLock",
    .description = "STC12 door lock controller",
    .resources = board_all_resources,
    .resourcesCnt = RES_COUNT,
    .is_critical = 1,
};

/* ---- per-input debounce state (initialised in board_bus_init) ---- */
static debounce_t doorDetectDb[DOORLOCK_DOOR_MAX];   /* 门磁 */
static debounce_t doorBtnDb[DOORLOCK_DOOR_MAX];      /* 门按键 */
static debounce_t cfgBtnDb[BUTTON_CNT];              /* 配置按键 */

/* ---- helpers ---- */

/* Build a uni-STC GpioConfig from a compact busManage_io_t. */
static void io_to_cfg(const busManage_io_t *io, GpioPinMode mode, GpioConfig *out)
{
    out->port = (GpioPort)io->port;
    out->pin = (GpioPin)io->pin;
    out->count = 1;
    out->pinMode = mode;
    out->setMask = (uint8_t)(1u << io->pin);
    out->clearMask = (uint8_t)~(1u << io->pin);
}

/* ---- resource init callbacks (run by the bus framework) ---- */

static int door_busInit(const busManage_resource_desc_t *res) REENTRANT
{
    const busManage_io_t *io = res->desc.custom.io_table;
    GpioConfig cfg;

    io_to_cfg(&io[0], GPIO_HIGH_IMPEDANCE_MODE, &cfg);   /* 门磁 detect：浮空输入 */
    gpioConfigure(&cfg);

    io_to_cfg(&io[1], GPIO_HIGH_IMPEDANCE_MODE, &cfg);   /* 门按键 door-button：浮空输入 */
    gpioConfigure(&cfg);

    io_to_cfg(&io[2], GPIO_PUSH_PULL_MODE, &cfg);        /* 继电器 relay output */
    gpioConfigure(&cfg);
    gpioWrite(&cfg, 1);                                  /* start locked */
    return 0;
}

static int button_busInit(const busManage_resource_desc_t *res) REENTRANT
{
    const busManage_io_t *io = res->desc.custom.io_table;
    unsigned short i;
    GpioConfig cfg;

    for (i = 0; i < res->desc.custom.io_count; i++) {
        io_to_cfg(&io[i], GPIO_HIGH_IMPEDANCE_MODE, &cfg);   /* 配置按键：浮空输入 */
        gpioConfigure(&cfg);
    }
    return 0;
}

static int debug_uart_busInit(const busManage_resource_desc_t *res) REENTRANT
{
    /* UART1 is brought up in main() before the bus framework runs, so the
     * debug console is already available for the init logging below. */
    (void)res;
    return 0;
}

/* ---- public interface ---- */

int board_bus_init(void)
{
    busManage_alloc_result_t ret;
    unsigned short i;

    ret = busManage_manager_init(&board_busModel);
    if (ret != BUS_ALLOC_OK) {
        ERROR_PRINT("bus manager_init failed: %d", (int)ret);
        return -1;
    }

    for (i = 0; i < board_busModel.resourcesCnt; i++) {
        const busManage_resource_desc_t *res = &board_busModel.resources[i];
        ret = busManage_manager_allocate(res);
        if (ret != BUS_ALLOC_OK && ret != BUS_ALLOC_ALREADY_DONE) {
            ERROR_PRINT("resource[%u] alloc failed: %d", i, (int)ret);
            return -1;
        }
    }

    /* Inputs idle high (external pull-up); debounce from that level. */
    for (i = 0; i < DOORLOCK_DOOR_MAX; i++) {
        debounce_init(&doorDetectDb[i], 1);
        debounce_init(&doorBtnDb[i], 1);
    }
    for (i = 0; i < BUTTON_CNT; i++)
        debounce_init(&cfgBtnDb[i], 1);

#ifdef BUSMANAGE_DEBUG
    busManage_manager_dump_status();
#endif
    return 0;
}

/* Shared method table: all doors use the same detect/lock callbacks.
 * Stored const (flash); each door's hwConfig only keeps a pointer to it. */
static const doorLock_ops_t door_ops = {
    .detect   = board_doorDetect,
    .lock     = board_doorLockCtrl,
    .doorInit = NULL,
};

void board_registerDoors(doorLock_manager_t *mgr)
{
    uint8_t i;

    for (i = 0; i < DOORLOCK_DOOR_MAX; i++) {
        const busManage_resource_desc_t *res = &board_all_resources[RES_DOOR0 + i];
        const busManage_io_t *io = res->desc.custom.io_table;
        doorLock_hwConfig_t cfg;
        cfg.io.detect     = io[0];   /* 门磁 */
        cfg.io.doorButton = io[1];   /* 门按键 */
        cfg.io.lock       = io[2];   /* 继电器 */
        cfg.time.lockDelaySec = LOCK_DEFAULT_DELAY_SEC;
        cfg.ops = &door_ops;
        doorLock_register(mgr, &cfg, res);
    }
}

doorLock_err_t board_doorDetect(doorLock_context_t *ctx) REENTRANT
{
    GpioConfig cfg;
    uint8_t level;

    io_to_cfg(&ctx->hw.io.detect, GPIO_HIGH_IMPEDANCE_MODE, &cfg);
    level = gpioRead(&cfg);

    if (level)
        doorLock_setWire(ctx);   /* line idle (high) */
    else
        doorLock_clrWire(ctx);   /* line triggered (low) */

    return doorLock_err_none;
}

doorLock_err_t board_doorLockCtrl(const doorLock_context_t *ctx, unsigned char lock) REENTRANT
{
    GpioConfig cfg;
    io_to_cfg(&ctx->hw.io.lock, GPIO_PUSH_PULL_MODE, &cfg);
    gpioWrite(&cfg, lock ? 1 : 0);
    return doorLock_err_none;
}

void board_bus_poll(void)
{
    static const busManage_io_t *door_io[DOORLOCK_DOOR_MAX] = {
        door0_io, door1_io, door2_io, door3_io
    };
    static uint16_t lastSample = 0;
    uint16_t now = userTMR_GetTick();
    uint8_t i;
    uint8_t detectMask = 0;
    uint8_t openMask = 0;
    GpioConfig cfg;

    /* Sample every BOARD_POLL_PERIOD_MS, independent of call frequency. */
    if ((uint16_t)(now - lastSample) < BOARD_POLL_PERIOD_MS)
        return;
    lastSample = now;

    /* 门磁: debounced edge (both directions) -> update door status. */
    for (i = 0; i < DOORLOCK_DOOR_MAX; i++) {
        io_to_cfg(&door_io[i][0], GPIO_HIGH_IMPEDANCE_MODE, &cfg);
        if (debounce_sample(&doorDetectDb[i], gpioRead(&cfg)))
            detectMask |= (uint8_t)(1U << i);
    }
    if (detectMask)
        doorLockTask_requestDetect(detectMask);

    /* 门按键: open on release (debounced low -> high). */
    for (i = 0; i < DOORLOCK_DOOR_MAX; i++) {
        io_to_cfg(&door_io[i][1], GPIO_HIGH_IMPEDANCE_MODE, &cfg);
        if (debounce_sample(&doorBtnDb[i], gpioRead(&cfg)) &&
            debounce_state(&doorBtnDb[i]) == 1)
            openMask |= (uint8_t)(1U << i);
    }
    if (openMask)
        doorLockTask_requestDoorButton(openMask);

    /* 配置键: debounced edge (both) -> feed the button framework. */
    for (i = 0; i < BUTTON_CNT; i++) {
        io_to_cfg(&button_io[i], GPIO_HIGH_IMPEDANCE_MODE, &cfg);
        if (debounce_sample(&cfgBtnDb[i], gpioRead(&cfg)))
            doorLockTask_onButton(i, debounce_state(&cfgBtnDb[i]));
    }
}
