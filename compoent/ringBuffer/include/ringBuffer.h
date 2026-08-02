#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef MIN
#define MIN(n, m) (((n) < (m)) ? (n) : (m))
#endif

#ifndef MAX
#define MAX(n, m) (((n) < (m)) ? (m) : (n))
#endif

typedef enum
{
    RINGBUF_OK = 0,
    RINGBUF_ERR_FAIL,
    RINGBUF_ERR_ARG,
    RINGBUF_ERR_BUF,
    RINGBUF_ERR_WR_DENIED,
    RINGBUF_ERR_INVALID_PTR,
    RINGBUF_ERR_EMPTY,
    RINGBUF_ERR_IDX,
} ringBuf_err_t;

typedef void *ringbuf_mutex_t;
typedef void (*ringbuf_lock_func_t)(void);
typedef void (*ringbuf_unlock_func_t)(void);

typedef unsigned int ringbuf_uidx_t;
typedef unsigned short ringbuf_ucnt_t;
typedef int ringbuf_idx_t;
typedef short ringbuf_cnt_t;

typedef struct ringbuf_t
{
    void *buffer;
    const ringbuf_ucnt_t depth;
    const ringbuf_ucnt_t item_size;

    volatile ringbuf_uidx_t wr_idx;
    volatile ringbuf_uidx_t rd_idx;

    bool overwritable;
} ringbuf_t;

typedef ptrdiff_t ringBuf_ptr_t;

#define RINGBUFCRTL_INIT(_buffer, _depth, _item_sz, _overwrite) \
    {                                                           \
        .buffer = (void *)(_buffer),                            \
        .depth = (_depth),                                      \
        .item_size = (_item_sz),                                \
        .wr_idx = 0,                                            \
        .rd_idx = 0,                                            \
        .overwritable = ((_overwrite) ? true : false),          \
    }

ringBuf_err_t ringBuf_clear(ringbuf_t *rb);
ringBuf_err_t ringBuf_count(const ringbuf_t *rb, ringbuf_cnt_t *pCount);
ringBuf_err_t ringBuf_init(ringbuf_t *rb);
ringBuf_err_t ringBuf_push(ringbuf_t *rb, const void *pData);
ringBuf_err_t ringBuf_pop(ringbuf_t *rb, void *pData);
ringBuf_err_t ringBuf_peek(const ringbuf_t *rb, void *pData, const ringbuf_ucnt_t itemIdx);
ringBuf_err_t ringBuf_push_multi(ringbuf_t *rb, const void *pData, const ringbuf_ucnt_t dataCount, ringbuf_cnt_t *pCount);
ringBuf_err_t ringBuf_pop_multi(ringbuf_t *rb, void *pData, const ringbuf_ucnt_t dataCount, ringbuf_cnt_t *pCount);
ringBuf_err_t ringBuf_peek_multi(const ringbuf_t *rb, void *pData, const ringbuf_ucnt_t dataCount,
                                 const ringbuf_cnt_t itemIdx, ringbuf_cnt_t *pCount);

#endif
