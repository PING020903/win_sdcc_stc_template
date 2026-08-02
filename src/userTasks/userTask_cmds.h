#ifndef _USERTASK_CMDS_H_
#define _USERTASK_CMDS_H_

/*
 * Stub of the original command task header.
 *
 * The CH58x project's command parser (CommandParse / AnyProtocolParser) is
 * too heavy for the STC12 and is not part of this build. Only the shared
 * scheduler globals that other modules reference are kept here.
 */

#include "EventSchedul.h"

extern EventSchedul_Context* evtSchedul_ctx;

#endif /* _USERTASK_CMDS_H_ */
