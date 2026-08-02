#include "project-defs.h"
#include "DBG_macro.h"
#include "CommandParseTree.h"
#include "reset-hal.h"
#include "userTask_cmds.h"
#include "userUART_init.h"

/*
 * Serial console command handling (see userTask_cmds.h).
 *
 * The cmdTree tokenizer needs a user-supplied allocator
 * (cmd_MemoryAlloc/Free). Its usage pattern is strictly "one live
 * allocation at a time, freed before the next parse", so a static pool
 * with a busy flag replaces the heap — STC12 has no room for one anyway.
 */

#define CMD_TOKEN_POOL_BYTES  24U

static uint8_t cmdTokenPool[CMD_TOKEN_POOL_BYTES];
static uint8_t cmdTokenPoolBusy = 0;

void *cmd_MemoryAlloc(size_t bytes)
{
    if (cmdTokenPoolBusy || bytes > CMD_TOKEN_POOL_BYTES)
        return NULL;
    cmdTokenPoolBusy = 1;
    return cmdTokenPool;
}

void cmd_MemoryFree(void *mem)
{
    if (mem == cmdTokenPool)
        cmdTokenPoolBusy = 0;
}

/* ---- receive buffer ---- */

#define CMD_POLL_PERIOD_MS  300U

static char cmdBuf[PARSE_SIZE];

/* ---- command handlers ---- */

static void cmd_reset(void *arg)
{
    (void)arg;
    DEBUG_PRINT("reset...");
    userUART_FlushTx();                 /* get the log out before resetting */
    softwareReset(SWRST_AP_area);       /* reboot into the user program */
    while (1)
        ;
}

/* Called from the door-lock task's TICK handler (see userTask_doorLock.c).
 * Every CMD_POLL_PERIOD_MS the RX FIFO is drained in one batch and fed to
 * the parser as-is — no line editing; CR/LF from terminals are dropped so
 * they don't end up inside tokens. */
void cmds_poll(void)
{
    static uint16_t pollCnt = 0;
    uint8_t len;
    uint8_t i;
    uint8_t j = 0;

    if (++pollCnt < CMD_POLL_PERIOD_MS)
        return;
    pollCnt = 0;

    len = userUART_ReadBuffer((uint8_t *)cmdBuf, (uint8_t)(PARSE_SIZE - 1U));
    if (len == 0)
        return;

    for (i = 0; i < len; i++) {             /* drop CR/LF in place */
        if (cmdBuf[i] != '\r' && cmdBuf[i] != '\n')
            cmdBuf[j++] = cmdBuf[i];
    }
    if (j == 0)
        return;

    cmdBuf[j] = '\0';
    cmdTree_CommandParse(cmdBuf);
}

int cmds_init(void)
{
    cmdTree_init();
    cmdTree_Register(CMDTREE_ROOT, "reset", cmd_reset, NULL);
    cmdTree_RegisterHelp(CMDTREE_ROOT);
    return 0;
}
