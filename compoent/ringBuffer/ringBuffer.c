#include "ringBuffer.h"
#include "DBG_macro.h"
#include <string.h>

#define RINGBUF_UPDATE_IDX(_idx, _depth) \
    do                                   \
    {                                    \
        (_idx)++;                        \
        if ((_idx) >= (2 * (_depth)))    \
        {                                \
            (_idx) -= (2 * (_depth));    \
        }                                \
    } while (0)

#define RINGBUF_ARG_CHECK(_rb)                                            \
    do                                                                    \
    {                                                                     \
        if (!(_rb))                                                       \
            return RINGBUF_ERR_ARG;                                       \
        if (!((_rb)->buffer))                                             \
            return RINGBUF_ERR_BUF;                                       \
        if (((_rb)->depth) == 0U || ((_rb)->item_size) == 0U)             \
            return RINGBUF_ERR_ARG;                                       \
        if ((ringbuf_uidx_t)((_rb)->depth) > ((ringbuf_uidx_t)-1) / 2U)  \
            return RINGBUF_ERR_ARG;                                       \
    } while (0)

__HIGH_CODE
static inline ringbuf_cnt_t _calc_count(ringbuf_uidx_t wr, ringbuf_uidx_t rd, ringbuf_ucnt_t depth) REENTRANT
{
    return (ringbuf_cnt_t)((wr >= rd) ? (wr - rd) : (wr + (ringbuf_uidx_t)depth - rd));
}

__HIGH_CODE
static inline void *_get_item_ptr(const ringbuf_t *rb, ringbuf_uidx_t idx) REENTRANT
{
    unsigned int actual_idx = idx % rb->depth;
    ringBuf_ptr_t ret = (ringBuf_ptr_t)rb->buffer + ((ringBuf_ptr_t)actual_idx * (ringBuf_ptr_t)rb->item_size);
    return (ret > 0) ? (void *)ret : NULL;
}

__HIGH_CODE
ringBuf_err_t ringBuf_clear(ringbuf_t *rb)
{
    RINGBUF_ARG_CHECK(rb);
    rb->rd_idx = 0U;
    rb->wr_idx = 0U;
    return RINGBUF_OK;
}

__HIGH_CODE
ringBuf_err_t ringBuf_count(const ringbuf_t *rb, ringbuf_cnt_t *pCount) REENTRANT
{
    RINGBUF_ARG_CHECK(rb);
    if (!pCount)
        return RINGBUF_ERR_ARG;
    *pCount = _calc_count(rb->wr_idx, rb->rd_idx, rb->depth);
    return RINGBUF_OK;
}

__HIGH_CODE
ringBuf_err_t ringBuf_init(ringbuf_t *rb)
{
    RINGBUF_ARG_CHECK(rb);
    rb->rd_idx = 0;
    rb->wr_idx = 0;
    return RINGBUF_OK;
}

__HIGH_CODE
ringBuf_err_t ringBuf_push(ringbuf_t *rb, const void *pData) REENTRANT
{
    RINGBUF_ARG_CHECK(rb);
    if (!pData)
        return RINGBUF_ERR_ARG;

    ringbuf_cnt_t count;
    ringBuf_err_t err = ringBuf_count(rb, &count);
    if (err != RINGBUF_OK)
        return err;

    if (count >= rb->depth)
    {
        if (!rb->overwritable)
            return RINGBUF_ERR_WR_DENIED;
        RINGBUF_UPDATE_IDX(rb->rd_idx, rb->depth);
    }

    void *write_pos = _get_item_ptr(rb, rb->wr_idx);
    if (!write_pos)
        return RINGBUF_ERR_INVALID_PTR;

    memcpy(write_pos, pData, rb->item_size);
    RINGBUF_UPDATE_IDX(rb->wr_idx, rb->depth);
    return RINGBUF_OK;
}

__HIGH_CODE
ringBuf_err_t ringBuf_pop(ringbuf_t *rb, void *pData) REENTRANT
{
    RINGBUF_ARG_CHECK(rb);
    if (!pData)
        return RINGBUF_ERR_ARG;

    ringbuf_cnt_t count;
    ringBuf_err_t err = ringBuf_count(rb, &count);
    if (err != RINGBUF_OK)
        return err;

    if (count == 0)
        return RINGBUF_ERR_EMPTY;

    void *read_pos = _get_item_ptr(rb, rb->rd_idx);
    memcpy(pData, read_pos, rb->item_size);
    RINGBUF_UPDATE_IDX(rb->rd_idx, rb->depth);
    return RINGBUF_OK;
}

__HIGH_CODE
ringBuf_err_t ringBuf_peek(const ringbuf_t *rb, void *pData, const ringbuf_ucnt_t itemIdx)
{
    RINGBUF_ARG_CHECK(rb);
    if (!pData)
        return RINGBUF_ERR_ARG;

    ringbuf_cnt_t count;
    ringBuf_err_t err = ringBuf_count(rb, &count);
    if (err != RINGBUF_OK)
        return err;

    if (count == 0)
        return RINGBUF_ERR_EMPTY;

    if (itemIdx >= count)
        return RINGBUF_ERR_IDX;

    ringbuf_uidx_t target_index = rb->rd_idx + itemIdx;
    if (target_index >= 2 * rb->depth)
        target_index -= 2 * rb->depth;

    void *read_pos = _get_item_ptr(rb, target_index);
    memcpy(pData, read_pos, rb->item_size);
    return RINGBUF_OK;
}

__HIGH_CODE
ringBuf_err_t ringBuf_push_multi(ringbuf_t *rb, const void *pData, const ringbuf_ucnt_t dataCount, ringbuf_cnt_t *pCount)
{
    if (!rb || !pData)
        return RINGBUF_ERR_ARG;

    const unsigned char *src = pData;
    ringbuf_cnt_t written = 0;
    ringBuf_err_t err = RINGBUF_OK;

    for (written = 0; written < (ringbuf_cnt_t)dataCount; written++)
    {
        err = ringBuf_push(rb, &src[written * rb->item_size]);
        if (err)
            break;
    }

    if (pCount)
        *pCount = written;
    return err;
}

__HIGH_CODE
ringBuf_err_t ringBuf_pop_multi(ringbuf_t *rb, void *pData, const ringbuf_ucnt_t dataCount, ringbuf_cnt_t *pCount)
{
    if (!rb || !pData)
        return RINGBUF_ERR_ARG;

    unsigned char *src = pData;
    ringbuf_cnt_t read_count = 0;
    ringBuf_err_t err = RINGBUF_OK;

    for (read_count = 0; read_count < (ringbuf_cnt_t)dataCount; read_count++)
    {
        err = ringBuf_pop(rb, &src[read_count * rb->item_size]);
        if (err)
            break;
    }

    if (pCount)
        *pCount = read_count;
    return err;
}

__HIGH_CODE
ringBuf_err_t ringBuf_peek_multi(const ringbuf_t *rb, void *pData, const ringbuf_ucnt_t dataCount, const ringbuf_cnt_t itemIdx, ringbuf_cnt_t *pCount)
{
    if (!rb || !pData)
        return RINGBUF_ERR_ARG;

    unsigned char *src = pData;
    ringbuf_cnt_t read_count = 0, target_index = 0;
    ringBuf_err_t err = RINGBUF_OK;

    for (read_count = 0; read_count < (ringbuf_cnt_t)dataCount; read_count++)
    {
        target_index = itemIdx + read_count;
        err = ringBuf_peek(rb, &src[read_count * rb->item_size], (ringbuf_ucnt_t)target_index);
        if (err)
            break;
    }

    if (pCount)
        *pCount = read_count;
    return err;
}
