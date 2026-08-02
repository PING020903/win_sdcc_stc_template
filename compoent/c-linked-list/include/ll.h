#ifndef LL_H__
#define LL_H__

#include <stdint.h>
#include <stddef.h>

/*
 * Minimal intrusive doubly-linked list type.
 *
 * The full c-linked-list library is only needed by EventSchedul's dynamic
 * mode. The STC12 build uses static mode, so only the ll_t node type is
 * required (it is embedded in EventSchedul_TaskNode).
 */

typedef struct ll_head {
	struct ll_head* next;
	struct ll_head* prev;
} ll_t;

#endif /* LL_H__ */
