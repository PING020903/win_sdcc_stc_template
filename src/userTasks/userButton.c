#include "userButton.h"
#include "DBG_macro.h"
#include <string.h>

void button_init(button_context_t *ctx, unsigned short longpressMs, void *buf, ringbuf_ucnt_t bufDepth)
{
    memset(ctx->ch, 0, sizeof(ctx->ch));
    ctx->longpressThreshold = longpressMs;

    ringbuf_t tmp = RINGBUFCRTL_INIT(buf, bufDepth, sizeof(button_rawEvt_t), false);
    memcpy(&ctx->evtQueue, &tmp, sizeof(ringbuf_t));
    ringBuf_init(&ctx->evtQueue);
}

__HIGH_CODE
void button_push(button_context_t *ctx, unsigned char idx, unsigned char level)
{
    button_rawEvt_t evt = { .idx = idx, .level = level };
    ringBuf_push(&ctx->evtQueue, &evt);
}

button_event_t button_poll(button_context_t *ctx, unsigned char *outIdx)
{
    button_rawEvt_t raw;
    if (ringBuf_pop(&ctx->evtQueue, &raw) != RINGBUF_OK)
        return button_event_none;

    if (raw.idx >= BUTTON_CNT)
        return button_event_none;

    button_channel_t *ch = &ctx->ch[raw.idx];

    if (raw.level == 0) {
        ch->pressed = 1;
        ch->holdCnt = 0;
        ch->longpressFired = 0;
        if (outIdx) *outIdx = raw.idx;
        return button_event_press;
    } else {
        ch->pressed = 0;
        ch->holdCnt = 0;
        if (outIdx) *outIdx = raw.idx;
        return button_event_release;
    }
}

void button_tick(button_context_t *ctx)
{
    int i;
    for (i = 0; i < BUTTON_CNT; i++) {
        if (!ctx->ch[i].pressed)
            continue;
        ctx->ch[i].holdCnt++;
    }
}

unsigned char button_isLongpress(button_context_t *ctx, unsigned char idx)
{
    button_channel_t *ch;
    if (idx >= BUTTON_CNT)
        return 0;
    ch = &ctx->ch[idx];
    if (ch->pressed && !ch->longpressFired && ch->holdCnt >= ctx->longpressThreshold) {
        ch->longpressFired = 1;
        return 1;
    }
    return 0;
}
