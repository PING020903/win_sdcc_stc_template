#ifndef _BOARD_BUS_H_
#define _BOARD_BUS_H_

#include "doorLock.h"

/* Run the bus framework: validate the board model and initialise every
 * resource's IO via its init callback. Returns 0 on success, -1 on error. */
int board_bus_init(void);

/* Register all doors with the door lock manager (placeholder wiring). */
void board_registerDoors(doorLock_manager_t *mgr);

/* Periodic polling hook: feeds door triggers and button edges to the task. */
void board_bus_poll(void);

/* doorLock hardware callbacks. */
doorLock_err_t board_doorDetect(doorLock_context_t *ctx) REENTRANT;
doorLock_err_t board_doorLockCtrl(const doorLock_context_t *ctx, unsigned char lock) REENTRANT;

#endif /* _BOARD_BUS_H_ */
