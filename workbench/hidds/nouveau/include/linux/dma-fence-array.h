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

#ifndef _LINUX_DMA_FENCE_ARRAY_H_
#define _LINUX_DMA_FENCE_ARRAY_H_

#include <linux/dma-fence.h>
#include <linux/irq_work.h>

struct dma_fence_array_cb {
	struct dma_fence_cb cb;
	struct dma_fence_array *array;
};

struct dma_fence_array {
	struct dma_fence base;
	spinlock_t lock;
	unsigned int num_fences;
	atomic_t num_pending;
	struct dma_fence **fences;
	struct irq_work work;
	struct dma_fence_array_cb callbacks[] __counted_by(num_fences);
};

#define dma_fence_array_for_each(fence, index, head)			\
	for (index = 0, fence = dma_fence_array_first(head);		\
	     fence != NULL;						\
	     (index)++, fence = dma_fence_array_next(head, index))

struct dma_fence_array *to_dma_fence_array(struct dma_fence *fence);
struct dma_fence_array *dma_fence_array_alloc(int num_fences);
void dma_fence_array_init(struct dma_fence_array *array,
    int num_fences, struct dma_fence **fences,
    u64 context, unsigned seqno,
    bool signal_on_any);
struct dma_fence_array *dma_fence_array_create(int num_fences,
    struct dma_fence **fences, u64 context, unsigned seqno,
    bool signal_on_any);
struct dma_fence *dma_fence_array_first(struct dma_fence *head);
struct dma_fence *dma_fence_array_next(struct dma_fence *head,
    unsigned int index);

#endif /* _LINUX_DMA_FENCE_ARRAY_H_ */
