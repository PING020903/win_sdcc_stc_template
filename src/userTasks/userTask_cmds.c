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

/* ---- line assembly ---- */

static char lineBuf[PARSE_SIZE];
static uint8_t lineLen = 0;

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

/* Called from the door-lock task's TICK handler (see userTask_doorLock.c). */
void cmds_poll(void)
{
    int16_t c;

    while ((c = userUART_ReadByte()) >= 0) {
        if (c == '\r' || c == '\n') {
            if (lineLen > 0) {
                lineBuf[lineLen] = '\0';
                cmdTree_CommandParse(lineBuf);
                lineLen = 0;
            }
        } else if (c == 0x08 || c == 0x7F) {    /* backspace */
            if (lineLen > 0)
                lineLen--;
        } else if (lineLen < (uint8_t)(PARSE_SIZE - 1U)) {
            lineBuf[lineLen++] = (char)c;
        }
    }
}

int cmds_init(void)
{
    cmdTree_init();
    cmdTree_Register(CMDTREE_ROOT, "reset", cmd_reset, NULL);
    cmdTree_RegisterHelp(CMDTREE_ROOT);
    return 0;
}
