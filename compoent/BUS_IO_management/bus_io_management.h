#ifndef _BUS_IO_MANAGEMENT_H_
#define _BUS_IO_MANAGEMENT_H_

/*
 * Trimmed bus / IO resource framework for the STC12 door lock.
 *
 * This keeps the declarative shape of the original CH58x bus manager —
 * a device model bundling resource descriptors, each carrying an init
 * callback — so board_bus.c can initialise every door's IO one resource
 * at a time through a uniform allocate loop.
 *
 * What was dropped for the 1 KB-XRAM 8051:
 *   - The runtime conflict-tracking state (per-bus instance tables, SPI CS
 *     table, baudrate/version arrays, ~120-140 B XRAM). This board is a
 *     single fixed design with statically-declared, mutually-distinct
 *     resources and no hot-plug, so bus arbitration can never trigger.
 *   - Bus types not used here (SPI / I2C / CAN / USB).
 *
 * The manager now only validates resources against the active model and
 * invokes their init callbacks; its runtime state is a model pointer plus
 * an initialised flag (~5 B).
 */

#include <stdint.h>
#include "DBG_macro.h"

typedef enum {
    BUS_ALLOC_OK = 0,
    BUS_ALLOC_CONFLICT,
    BUS_ALLOC_NOT_AVAILABLE,
    BUS_ALLOC_INVALID_ARG,
    BUS_ALLOC_ALREADY_DONE,
    BUS_ALLOC_INIT_FAIL
} busManage_alloc_result_t;

typedef enum {
    mcu_busType_UART = 1,
    mcu_busType_custom,
    mcu_busType_MAX
} busManage_bus_type_t;

/* STC12 ports P0..P3; values match uni-STC GPIO_PORTx. */
#define BUSIO_PORT_P0  0
#define BUSIO_PORT_P1  1
#define BUSIO_PORT_P2  2
#define BUSIO_PORT_P3  3

/* Compact GPIO identifier packed in one byte: pin in the low nibble
 * (0-7), port in the high nibble (BUSIO_PORT_Px). SDCC lays bit-fields out
 * from the least-significant bit, so declaring pin first puts it low. */
typedef struct {
    uint8_t pin  : 4;   /* 0-7 */
    uint8_t port : 4;   /* BUSIO_PORT_Px */
} busManage_io_t;

typedef struct {
    unsigned short io_count;
    const busManage_io_t *io_table;
    const char *descString;
} busManage_custom_desc_t;

typedef struct {
    unsigned short uart_instance;
    unsigned long baudrate;   /* 32-bit: 8051 unsigned int is only 16-bit */
    const char *descString;
} busManage_uart_desc_t;

struct busManage_resource_desc;

/* Init callback. Invoked through a pointer, so it must be reentrant on 8051. */
typedef int (*busManage_initFn)(const struct busManage_resource_desc *res) REENTRANT;

typedef struct busManage_resource_desc {
    busManage_bus_type_t type;
    union {
        busManage_uart_desc_t uart;
        busManage_custom_desc_t custom;
    } desc;
    busManage_initFn init;
} busManage_resource_desc_t;

typedef struct {
    const char *device_name;
    const char *description;
    const busManage_resource_desc_t *resources;
    const unsigned short resourcesCnt;
    const unsigned short is_critical;
} busManage_bus_model_t;

busManage_alloc_result_t busManage_manager_init(const busManage_bus_model_t *model);
busManage_alloc_result_t busManage_manager_allocate(const busManage_resource_desc_t *bus);
busManage_alloc_result_t busManage_manager_release(const busManage_resource_desc_t *bus);

#ifdef BUSMANAGE_DEBUG
void busManage_manager_dump_status(void);
#endif

#endif /* _BUS_IO_MANAGEMENT_H_ */
