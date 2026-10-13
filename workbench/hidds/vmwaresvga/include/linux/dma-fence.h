/*-
 * Copyright (c) 2022 Beckhoff Automation GmbH & Co. KG
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef __LINUX_DMA_FENCE_H_
#define __LINUX_DMA_FENCE_H_

#include <linux/err.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/bitops.h>
#include <linux/kref.h>
#include <linux/sched.h>
#include <linux/printk.h>
#include <linux/rcupdate.h>
#include <linux/timekeeping.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

struct dma_fence_ops;
struct seq_file;

struct dma_fence {
	spinlock_t *lock;
	const struct dma_fence_ops *ops;
	union {
		struct list_head cb_list;
		ktime_t timestamp;
		struct rcu_head rcu;
	};
	u64 context;
	u64 seqno;
	unsigned long flags;
	struct kref refcount;
	int error;
};

struct dma_fence_cb;

typedef void (*dma_fence_func_t)(struct dma_fence *fence,
				 struct dma_fence_cb *cb);

struct dma_fence_cb {
	struct list_head node;
	dma_fence_func_t func;
};

struct dma_fence_ops {
	bool use_64bit_seqno;

	const char * (*get_driver_name)(struct dma_fence *fence);
	const char * (*get_timeline_name)(struct dma_fence *fence);
	bool (*enable_signaling)(struct dma_fence *fence);
	bool (*signaled)(struct dma_fence *fence);
	signed long (*wait)(struct dma_fence *fence,
			    bool intr, signed long timeout);
	void (*release)(struct dma_fence *fence);
	void (*fence_value_str)(struct dma_fence *fence, char *str, int size);
	void (*timeline_value_str)(struct dma_fence *fence,
				   char *str, int size);
	void (*set_deadline)(struct dma_fence *fence, ktime_t deadline);
};

enum dma_fence_flag_bits {
	DMA_FENCE_FLAG_SIGNALED_BIT,
	DMA_FENCE_FLAG_TIMESTAMP_BIT,
	DMA_FENCE_FLAG_ENABLE_SIGNAL_BIT,
	DMA_FENCE_FLAG_USER_BITS, /* must always be last member */
};

extern const struct dma_fence_ops dma_fence_array_ops;
extern const struct dma_fence_ops dma_fence_chain_ops;

void dma_fence_init(struct dma_fence *fence, const struct dma_fence_ops *ops,
    spinlock_t *lock, u64 context, u64 seqno);
void dma_fence_release(struct kref *kref);
void dma_fence_free(struct dma_fence *fence);
void dma_fence_describe(struct dma_fence *fence, struct seq_file *seq);
int dma_fence_signal(struct dma_fence *fence);
int dma_fence_signal_locked(struct dma_fence *fence);
int dma_fence_signal_timestamp(struct dma_fence *fence, ktime_t timestamp);
int dma_fence_signal_timestamp_locked(struct dma_fence *fence,
				      ktime_t timestamp);
signed long dma_fence_default_wait(struct dma_fence *fence,
    bool intr, signed long timeout);
int dma_fence_add_callback(struct dma_fence *fence,
    struct dma_fence_cb *cb, dma_fence_func_t func);
bool dma_fence_remove_callback(struct dma_fence *fence,
    struct dma_fence_cb *cb);
void dma_fence_enable_sw_signaling(struct dma_fence *fence);
int dma_fence_get_status(struct dma_fence *fence);
signed long dma_fence_wait_timeout(struct dma_fence *,
    bool intr, signed long timeout);
signed long dma_fence_wait_any_timeout(struct dma_fence **fences,
    uint32_t count, bool intr, signed long timeout, uint32_t *idx);
void dma_fence_set_deadline(struct dma_fence *fence, ktime_t deadline);
struct dma_fence *dma_fence_get_stub(void);
struct dma_fence *dma_fence_allocate_private_stub(ktime_t timestamp);
u64 dma_fence_context_alloc(unsigned num);
void dma_fence_put(struct dma_fence *fence);
struct dma_fence *dma_fence_get(struct dma_fence *fence);
struct dma_fence *dma_fence_get_rcu(struct dma_fence *fence);
struct dma_fence *dma_fence_get_rcu_safe(struct dma_fence __rcu **fencep);
bool dma_fence_is_signaled_locked(struct dma_fence *fence);
bool dma_fence_is_signaled(struct dma_fence *fence);
bool __dma_fence_is_later(uint64_t f1, uint64_t f2,
    const struct dma_fence_ops *ops);
bool dma_fence_is_later(struct dma_fence *f1, struct dma_fence *f2);
bool dma_fence_is_later_or_same(struct dma_fence *f1, struct dma_fence *f2);
struct dma_fence *dma_fence_later(struct dma_fence *f1, struct dma_fence *f2);
int dma_fence_get_status_locked(struct dma_fence *fence);
void dma_fence_set_error(struct dma_fence *fence, int error);
ktime_t dma_fence_timestamp(struct dma_fence *fence);
signed long dma_fence_wait(struct dma_fence *fence, bool intr);
bool dma_fence_is_array(struct dma_fence *fence);
bool dma_fence_is_chain(struct dma_fence *fence);
static inline bool
dma_fence_is_container(struct dma_fence *fence)
{
	return (dma_fence_is_array(fence) || dma_fence_is_chain(fence));
}

#define	dma_fence_begin_signalling() true
#define	dma_fence_end_signalling(cookie) do { (void)cookie; } while (0)

#define	__dma_fence_might_wait	(void)

#endif
