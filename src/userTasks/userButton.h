#ifndef _USERBUTTON_H_
#define _USERBUTTON_H_

#include "bus_io_management.h"
#include "ringBuffer.h"

/* 8 unified channels: per-door buttons 0..3, global config keys 4..7. */
#define BUTTON_CNT 8
#define BUTTON_EVT_QUEUE_DEPTH 8

typedef enum {
    button_event_none = 0,
    button_event_press,
    button_event_release,
    button_event_longpress,
} button_event_t;

typedef struct {
    unsigned char idx;
    unsigned char level;
} button_rawEvt_t;

typedef struct {
    unsigned char pressed;
    unsigned short holdCnt;
    unsigned char longpressFired;
} button_channel_t;

typedef struct {
    button_channel_t ch[BUTTON_CNT];
    unsigned short longpressThreshold;
    ringbuf_t evtQueue;
} button_context_t;

void button_init(button_context_t *ctx, unsigned short longpressMs, void *buf, ringbuf_ucnt_t bufDepth);
void button_push(button_context_t *ctx, unsigned char idx, unsigned char level);
button_event_t button_poll(button_context_t *ctx, unsigned char *outIdx);
void button_tick(button_context_t *ctx);
unsigned char button_isLongpress(button_context_t *ctx, unsigned char idx);

#endif
