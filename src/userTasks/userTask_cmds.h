#ifndef _USERTASK_CMDS_H_
#define _USERTASK_CMDS_H_

/*
 * Serial console command task.
 *
 * Bytes received by the UART1 ISR land in the RX FIFO; this task drains
 * them on every 1 ms tick, assembles lines and feeds complete lines to
 * the cmdTree command parser (compoent/CommandParse, static mode).
 *
 * Registered commands:
 *   reset  software reset (IAP_CONTR), restarts the firmware so the full
 *          boot log can be captured again without a hardware reset button
 *   help   built-in, lists registered commands
 */

#include "EventSchedul.h"

extern EventSchedul_Context* evtSchedul_ctx;

int cmds_init(void);

#endif /* _USERTASK_CMDS_H_ */
