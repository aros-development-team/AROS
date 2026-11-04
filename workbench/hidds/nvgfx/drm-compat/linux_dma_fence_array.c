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


#include <linux/dma-fence-array.h>
#include <linux/spinlock.h>

#include <linux/slab.h>

static const char *
dma_fence_array_get_driver_name(struct dma_fence *fence)
{

	return ("dma_fence_array");
}

static const char *
dma_fence_array_get_timeline_name(struct dma_fence *fence)
{

	return ("unbound");
}

static void
irq_dma_fence_array_work(struct irq_work *work)
{
	struct dma_fence_array *array;

	array = container_of(work, typeof(*array), work);

	dma_fence_signal(&array->base);
	dma_fence_put(&array->base);
}

static void
dma_fence_array_cb_func(struct dma_fence *f, struct dma_fence_cb *cb)
{
	struct dma_fence_array_cb *array_cb;

	array_cb = container_of(cb, struct dma_fence_array_cb, cb);
	if (atomic_dec_and_test(&array_cb->array->num_pending))
		irq_work_queue(&array_cb->array->work);
	else
		dma_fence_put(&array_cb->array->base);
}

static bool
dma_fence_array_enable_signaling(struct dma_fence *fence)
{
	struct dma_fence_array *array;
	struct dma_fence_array_cb *cb;
	int i;

	array = to_dma_fence_array(fence);
	cb = array->callbacks;
	if (array == NULL)
		return (false);

	for (i = 0; i < array->num_fences; i++) {
		cb[i].array = array;
		dma_fence_get(&array->base);
		if (dma_fence_add_callback(array->fences[i], &cb[i].cb,
		    dma_fence_array_cb_func)) {
			dma_fence_put(&array->base);
			if (atomic_dec_and_test(&array->num_pending))
				return (false);
		}
	}

	return (true);
}

static bool
dma_fence_array_signaled(struct dma_fence *fence)
{
	struct dma_fence_array *array;

	array = to_dma_fence_array(fence);
	if (array == NULL)
		return (false);

	return (atomic_read(&array->num_pending) <= 0);
}

static void
dma_fence_array_release(struct dma_fence *fence)
{
	struct dma_fence_array *array;
	int i;

	array = to_dma_fence_array(fence);
	if (array == NULL)
		return;

	for (i = 0; i < array->num_fences; i++)
		dma_fence_put(array->fences[i]);

	kfree(array->fences);
	dma_fence_free(fence);
}

struct dma_fence *
dma_fence_array_first(struct dma_fence *head)
{
	struct dma_fence_array *array;

	if (head == NULL)
		return (NULL);

	if ((array = to_dma_fence_array(head)) == NULL)
		return (head);

	if (array->num_fences > 0)
		return (array->fences[0]);

	return (NULL);
}

struct dma_fence *
dma_fence_array_next(struct dma_fence *head, unsigned int index)
{
	struct dma_fence_array *array;

	if (head == NULL)
		return (NULL);

	if ((array = to_dma_fence_array(head)) == NULL)
		return (NULL);

	if (index < array->num_fences)
		return (array->fences[index]);

	return (NULL);
}

static void
dma_fence_array_set_deadline(struct dma_fence *fence, ktime_t deadline)
{
	struct dma_fence_array *array;
	int i;

	array = to_dma_fence_array(fence);
	for (i = 0; i < array->num_fences; i++)
		dma_fence_set_deadline(array->fences[i], deadline);
}

const struct dma_fence_ops dma_fence_array_ops = {
	.get_driver_name = dma_fence_array_get_driver_name,
	.get_timeline_name = dma_fence_array_get_timeline_name,
	.enable_signaling = dma_fence_array_enable_signaling,
	.signaled = dma_fence_array_signaled,
	.release = dma_fence_array_release,
	.set_deadline = dma_fence_array_set_deadline,
};

struct dma_fence_array *
dma_fence_array_alloc(int num_fences)
{
	struct dma_fence_array *array;

	array = kzalloc(struct_size(array, callbacks, num_fences), GFP_KERNEL);

	return (array);
}

void
dma_fence_array_init(struct dma_fence_array *array,
    int num_fences, struct dma_fence **fences,
    u64 context, unsigned seqno,
    bool signal_on_any)
{
	array->num_fences = num_fences;
	spin_lock_init(&array->lock);
	dma_fence_init(&array->base, &dma_fence_array_ops,
	  &array->lock, context, seqno);
	init_irq_work(&array->work, irq_dma_fence_array_work);
	atomic_set(&array->num_pending, signal_on_any ? 1 : num_fences);
	array->fences = fences;
}

/*
 * Create a custom fence array
 */
struct dma_fence_array *
dma_fence_array_create(int num_fences,
    struct dma_fence **fences,
    u64 context, unsigned seqno,
    bool signal_on_any)
{
	struct dma_fence_array *array;

	array = dma_fence_array_alloc(num_fences);
	if (!array)
		return (NULL);

	dma_fence_array_init(array, num_fences, fences, context, seqno,
	    signal_on_any);

	return (array);
}

/*
 * check if a fence is from the array subsclass
 */
bool dma_fence_is_array(struct dma_fence *fence)
{

	return (fence->ops == &dma_fence_array_ops);
}

/*
 * cast a fence to a dma_fence_array
 */
struct dma_fence_array *
to_dma_fence_array(struct dma_fence *fence)
{

	if (fence == NULL || !dma_fence_is_array(fence))
		return NULL;

	return (container_of(fence, struct dma_fence_array, base));
}
