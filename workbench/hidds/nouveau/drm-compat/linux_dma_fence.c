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

#include <linux/dma-fence.h>
#include <linux/delay.h>
#include <linux/seq_file.h>

#include <linux/slab.h>

static struct dma_fence dma_fence_stub;
static DEFINE_SPINLOCK(dma_fence_stub_lock);

static const char *
dma_fence_stub_get_name(struct dma_fence *fence)
{

	return ("stub");
}

static const struct dma_fence_ops dma_fence_stub_ops = {
	.get_driver_name = dma_fence_stub_get_name,
	.get_timeline_name = dma_fence_stub_get_name,
};

/*
 * return a signaled fence
 */
struct dma_fence *
dma_fence_get_stub(void)
{

	spin_lock(&dma_fence_stub_lock);
	if (dma_fence_stub.ops == NULL) {
		dma_fence_init(&dma_fence_stub,
		    &dma_fence_stub_ops,
		    &dma_fence_stub_lock,
		    0,
		    0);
		set_bit(DMA_FENCE_FLAG_ENABLE_SIGNAL_BIT,
		    &dma_fence_stub.flags);
		dma_fence_signal_locked(&dma_fence_stub);
	}
	spin_unlock(&dma_fence_stub_lock);
	return (dma_fence_get(&dma_fence_stub));
}

struct dma_fence *
dma_fence_allocate_private_stub(ktime_t timestamp)
{
	struct dma_fence *fence;

	fence = kzalloc(sizeof(*fence), GFP_KERNEL);
	if (fence == NULL)
		return (NULL);

	dma_fence_init(fence,
	    &dma_fence_stub_ops, &dma_fence_stub_lock, 0, 0);
	set_bit(DMA_FENCE_FLAG_ENABLE_SIGNAL_BIT, &fence->flags);
	dma_fence_signal_timestamp(fence, timestamp);

	return (fence);
}

static atomic64_t dma_fence_context_counter = ATOMIC64_INIT(1);

/*
 * allocate an array of fence contexts
 */
u64
dma_fence_context_alloc(unsigned num)
{

	return (atomic64_fetch_add(num, &dma_fence_context_counter));
}

/*
 * signal completion of a fence
 */
int
dma_fence_signal_timestamp_locked(struct dma_fence *fence,
				  ktime_t timestamp)
{
	struct dma_fence_cb *cur, *tmp;
	struct list_head cb_list;

	if (fence == NULL)
		return (-EINVAL);
	if (test_and_set_bit(DMA_FENCE_FLAG_SIGNALED_BIT,
	      &fence->flags))
		return (-EINVAL);

	list_replace(&fence->cb_list, &cb_list);

	fence->timestamp = timestamp;
	set_bit(DMA_FENCE_FLAG_TIMESTAMP_BIT, &fence->flags);

	list_for_each_entry_safe(cur, tmp, &cb_list, node) {
		INIT_LIST_HEAD(&cur->node);
		cur->func(fence, cur);
	}

	return (0);
}

/*
 * signal completion of a fence
 */
int
dma_fence_signal_timestamp(struct dma_fence *fence, ktime_t timestamp)
{
	int rv;

	if (fence == NULL)
		return (-EINVAL);

	spin_lock(fence->lock);
	rv = dma_fence_signal_timestamp_locked(fence, timestamp);
	spin_unlock(fence->lock);
	return (rv);
}

/*
 * signal completion of a fence
 */
int
dma_fence_signal_locked(struct dma_fence *fence)
{
	return dma_fence_signal_timestamp_locked(fence, ktime_get());
}

/*
 * signal completion of a fence
 */
int
dma_fence_signal(struct dma_fence *fence)
{
	int rv;

	if (fence == NULL)
		return (-EINVAL);

	spin_lock(fence->lock);
	rv = dma_fence_signal_timestamp_locked(fence, ktime_get());
	spin_unlock(fence->lock);
	return (rv);
}

/*
 * get completion timestamp of a fence
 */
ktime_t
dma_fence_timestamp(struct dma_fence *fence)
{
	if (!test_bit(DMA_FENCE_FLAG_SIGNALED_BIT, &fence->flags))
		return (ktime_get());

	while (!test_bit(DMA_FENCE_FLAG_TIMESTAMP_BIT, &fence->flags))
		cpu_relax();

	return (fence->timestamp);
}

/*
 * sleep until the fence gets signaled or until timeout elapses
 */
signed long
dma_fence_wait_timeout(struct dma_fence *fence, bool intr, signed long timeout)
{
	signed long rv;

	if (fence == NULL)
		return (-EINVAL);

	dma_fence_enable_sw_signaling(fence);

	if (fence->ops && fence->ops->wait != NULL)
		rv = fence->ops->wait(fence, intr, timeout);
	else
		rv = dma_fence_default_wait(fence, intr, timeout);
	return (rv);
}

/*
 * default relese function for fences
 */
void
dma_fence_release(struct kref *kref)
{
	struct dma_fence *fence;

	fence = container_of(kref, struct dma_fence, refcount);
	if (fence->ops && fence->ops->release)
		fence->ops->release(fence);
	else
		dma_fence_free(fence);
}

/*
 * default release function for dma_fence
 */
void
dma_fence_free(struct dma_fence *fence)
{

	kfree_rcu(fence, rcu);
}

/*
 * enable signaling on fence
 */
void
dma_fence_enable_sw_signaling(struct dma_fence *fence)
{
	bool was_enabled;

	spin_lock(fence->lock);
	was_enabled = test_and_set_bit(DMA_FENCE_FLAG_ENABLE_SIGNAL_BIT,
	    &fence->flags);
	if (test_bit(DMA_FENCE_FLAG_SIGNALED_BIT, &fence->flags))
		goto out;
	if (was_enabled == false &&
	    fence->ops && fence->ops->enable_signaling) {
		if (fence->ops->enable_signaling(fence) == false)
			dma_fence_signal_locked(fence);
	}
out:
	spin_unlock(fence->lock);
}

/*
 * add a callback to be called when the fence is signaled
 */
int
dma_fence_add_callback(struct dma_fence *fence, struct dma_fence_cb *cb,
			   dma_fence_func_t func)
{
	int rv = 0;
	bool was_enabled;

	if (fence == NULL || func == NULL)
		return (-EINVAL);

	if (test_bit(DMA_FENCE_FLAG_SIGNALED_BIT, &fence->flags)) {
		INIT_LIST_HEAD(&cb->node);
		return (-ENOENT);
	}

	spin_lock(fence->lock);
	was_enabled = test_and_set_bit(DMA_FENCE_FLAG_ENABLE_SIGNAL_BIT,
	    &fence->flags);

	if (test_bit(DMA_FENCE_FLAG_SIGNALED_BIT, &fence->flags))
		rv = -ENOENT;
	else if (was_enabled == false && fence->ops
	    && fence->ops->enable_signaling) {
		if (!fence->ops->enable_signaling(fence)) {
			dma_fence_signal_locked(fence);
			rv = -ENOENT;
		}
	}

	if (!rv) {
		cb->func = func;
		list_add_tail(&cb->node, &fence->cb_list);
	} else
		INIT_LIST_HEAD(&cb->node);
	spin_unlock(fence->lock);

	return (rv);
}

/*
 * returns the status upon completion
 */
int
dma_fence_get_status(struct dma_fence *fence)
{
	int rv;

	spin_lock(fence->lock);
	rv = dma_fence_get_status_locked(fence);
	spin_unlock(fence->lock);
	return (rv);
}

/*
 * remove a callback from the signaling list
 */
bool
dma_fence_remove_callback(struct dma_fence *fence, struct dma_fence_cb *cb)
{
	int rv;

	spin_lock(fence->lock);
	rv = !list_empty(&cb->node);
	if (rv)
		list_del_init(&cb->node);
	spin_unlock(fence->lock);
	return (rv);
}


struct default_wait_cb {
	struct dma_fence_cb base;
	struct task_struct *task;
};

static void
dma_fence_default_wait_cb(struct dma_fence *fence, struct dma_fence_cb *cb)
{
	struct default_wait_cb *wait =
		container_of(cb, struct default_wait_cb, base);

	wake_up_state(wait->task, TASK_NORMAL);
}

/*
 * default sleep until the fence gets signaled or until timeout elapses
 */
signed long
dma_fence_default_wait(struct dma_fence *fence, bool intr, signed long timeout)
{
	struct default_wait_cb cb;
	signed long rv = timeout ? timeout : 1;

	spin_lock(fence->lock);

	if (test_bit(DMA_FENCE_FLAG_SIGNALED_BIT, &fence->flags))
		goto out;

	if (timeout == 0) {
		rv = 0;
		goto out;
	}

	cb.base.func = dma_fence_default_wait_cb;
	cb.task = current;
	list_add(&cb.base.node, &fence->cb_list);

	/*
	 * Sleep in short slices and re-check the fence between them: a
	 * fence whose completion interrupt never arrives is still seen once
	 * the engine has written its sequence, instead of the whole timeout
	 * being spent. Long waits are reported once, with the interrupt
	 * count, so a broken interrupt path shows up in the log.
	 */
	{
		const signed long maxslice = msecs_to_jiffies(10) ? msecs_to_jiffies(10) : 1;
		signed long slice = 1;
		signed long waited = 0;

		/*
		 * The engine may already be done, or be about to be: most 2D
		 * work completes within microseconds, so look a few times
		 * before paying for a sleep at all (as the earlier port did).
		 */
		spin_unlock(fence->lock);
		{
			int spins;

			for (spins = 0; spins < 12; spins++) {
				if (dma_fence_is_signaled(fence))
					break;
				udelay(spins < 4 ? 5 : 20);
			}
		}
		spin_lock(fence->lock);

		while (!test_bit(DMA_FENCE_FLAG_SIGNALED_BIT, &fence->flags) && rv > 0) {
			signed long chunk = rv < slice ? rv : slice, left;

			if (slice < maxslice)
				slice *= 2;

			if (intr)
				__set_current_state(TASK_INTERRUPTIBLE);
			else
				__set_current_state(TASK_UNINTERRUPTIBLE);
			spin_unlock(fence->lock);

			left = schedule_timeout(chunk);
			waited += chunk - left;
			rv -= chunk - left;
			if (rv < 0)
				rv = 0;

			spin_lock(fence->lock);
			if (rv > 0 && intr && signal_pending(current))
				rv = -ERESTARTSYS;
		}
		if (rv <= 0 && waited && timeout != MAX_SCHEDULE_TIMEOUT)
			rv = 0;
		if (rv > 0 || test_bit(DMA_FENCE_FLAG_SIGNALED_BIT, &fence->flags))
			if (rv <= 0)
				rv = 1;
	}

	if (!list_empty(&cb.base.node))
		list_del(&cb.base.node);
	__set_current_state(TASK_RUNNING);
out:
	spin_unlock(fence->lock);
	return (rv);
}

static bool
dma_fence_test_signaled_any(struct dma_fence **fences, uint32_t count,
    uint32_t *idx)
{
	int i;

	for (i = 0; i < count; ++i) {
		struct dma_fence *fence = fences[i];
		if (test_bit(DMA_FENCE_FLAG_SIGNALED_BIT, &fence->flags)) {
			if (idx)
				*idx = i;
			return true;
		}
	}
	return false;
}

/*
 * sleep until any fence gets signaled or until timeout elapses
 */
signed long
dma_fence_wait_any_timeout(struct dma_fence **fences, uint32_t count,
			   bool intr, signed long timeout, uint32_t *idx)
{
	struct default_wait_cb *cb;
	long rv = timeout;
	int i;

	if (timeout == 0) {
		for (i = 0; i < count; i++) {
			if (dma_fence_is_signaled(fences[i])) {
				if (idx)
					*idx = i;
				return (1);
			}
		}
		return (0);
	}

	cb = kcalloc(count, sizeof(*cb), GFP_KERNEL);
	for (i = 0; i < count; i++) {
		struct dma_fence *fence = fences[i];
		cb[i].task = current;
		if (dma_fence_add_callback(fence, &cb[i].base,
		    dma_fence_default_wait_cb)) {
			if (idx)
				*idx = i;
			goto cb_cleanup;
		}
	}

	while (rv > 0) {
		if (intr)
			set_current_state(TASK_INTERRUPTIBLE);
		else
			set_current_state(TASK_UNINTERRUPTIBLE);

		if (dma_fence_test_signaled_any(fences, count, idx))
			break;

		rv = schedule_timeout(rv);

		if (rv > 0 && intr && signal_pending(current))
			rv = -ERESTARTSYS;
	}

	__set_current_state(TASK_RUNNING);

cb_cleanup:
	while (i-- > 0)
		dma_fence_remove_callback(fences[i], &cb[i].base);
	kfree(cb);
	return (rv);
}

void
dma_fence_set_deadline(struct dma_fence *fence, ktime_t deadline)
{
	if (fence->ops->set_deadline != NULL && !dma_fence_is_signaled(fence))
		fence->ops->set_deadline(fence, deadline);
}

void
dma_fence_describe(struct dma_fence *fence, struct seq_file *seq)
{
	seq_printf(seq, "%s %s seq %llu %ssignalled\n",
		   fence->ops->get_driver_name(fence),
		   fence->ops->get_timeline_name(fence), fence->seqno,
		   dma_fence_is_signaled(fence) ? "" : "un");
}

/*
 * Initialize a custom fence.
 */
void
dma_fence_init(struct dma_fence *fence, const struct dma_fence_ops *ops,
    spinlock_t *lock, u64 context, u64 seqno)
{

	kref_init(&fence->refcount);
	INIT_LIST_HEAD(&fence->cb_list);
	fence->ops = ops;
	fence->lock = lock;
	fence->context = context;
	fence->seqno = seqno;
	fence->flags = 0;
	fence->error = 0;
}

/*
 * decreases refcount of the fence
 */
void
dma_fence_put(struct dma_fence *fence)
{

	if (fence)
		kref_put(&fence->refcount, dma_fence_release);
}

/* 
 * increases refcount of the fence
 */
struct dma_fence *
dma_fence_get(struct dma_fence *fence)
{

	if (fence)
		kref_get(&fence->refcount);
	return (fence);
}

/*
 * get a fence from a dma_resv_list with rcu read lock
 */
struct dma_fence *
dma_fence_get_rcu(struct dma_fence *fence)
{

	if (kref_get_unless_zero(&fence->refcount))
		return (fence);
	else
		return (NULL);
}

/*
 * acquire a reference to an RCU tracked fence
 */
struct dma_fence *
dma_fence_get_rcu_safe(struct dma_fence __rcu **fencep)
{

	do {
		struct dma_fence *fence;

		fence = rcu_dereference(*fencep);
		if (!fence)
			return (NULL);

		if (!dma_fence_get_rcu(fence))
			continue;

		if (fence == rcu_access_pointer(*fencep))
			return rcu_pointer_handoff(fence);

		dma_fence_put(fence);
	} while (1);
}

/*
 * Return an indication if the fence is signaled yet.
 */
bool
dma_fence_is_signaled_locked(struct dma_fence *fence)
{

	if (test_bit(DMA_FENCE_FLAG_SIGNALED_BIT, &fence->flags))
		return (true);

	if (fence->ops->signaled && fence->ops->signaled(fence)) {
		dma_fence_signal_locked(fence);
		return (true);
	}

	return (false);
}

/*
 * Return an indication if the fence is signaled yet.
 */
bool
dma_fence_is_signaled(struct dma_fence *fence)
{

	if (test_bit(DMA_FENCE_FLAG_SIGNALED_BIT, &fence->flags))
		return (true);

	if (fence->ops->signaled && fence->ops->signaled(fence)) {
		dma_fence_signal(fence);
		return (true);
	}

	return (false);
}

/*
 * return if f1 is chronologically later than f2
 */
bool
__dma_fence_is_later(u64 f1, u64 f2, const struct dma_fence_ops *ops)
{

	if (ops->use_64bit_seqno)
		return (f1 > f2);

	return (int)(lower_32_bits(f1) - lower_32_bits(f2)) > 0;
}

/*
 * return if f1 is chronologically later than f2
 */
bool
dma_fence_is_later(struct dma_fence *f1,
    struct dma_fence *f2)
{

	if (WARN_ON(f1->context != f2->context))
		return (false);

	return (__dma_fence_is_later(f1->seqno, f2->seqno, f1->ops));
}

bool
dma_fence_is_later_or_same(struct dma_fence *f1, struct dma_fence *f2)
{

	return (f1 == f2 || dma_fence_is_later(f1, f2));
}

/*
 * return the chronologically later fence
 */
struct dma_fence *
dma_fence_later(struct dma_fence *f1,
    struct dma_fence *f2)
{
	if (WARN_ON(f1->context != f2->context))
		return (NULL);

	if (dma_fence_is_later(f1, f2))
		return (dma_fence_is_signaled(f1) ? NULL : f1);
	else
		return (dma_fence_is_signaled(f2) ? NULL : f2);
}

/*
 * returns the status upon completion
 */
int
dma_fence_get_status_locked(struct dma_fence *fence)
{

	assert_spin_locked(fence->lock);
	if (dma_fence_is_signaled_locked(fence))
		return (fence->error ?: 1);
	else
		return (0);
}

/*
 * flag an error condition on the fence
 */
void
dma_fence_set_error(struct dma_fence *fence,
    int error)
{

	fence->error = error;
}

/*
 * sleep until the fence gets signaled
 */
signed long
dma_fence_wait(struct dma_fence *fence, bool intr)
{
	signed long ret;

	ret = dma_fence_wait_timeout(fence, intr, MAX_SCHEDULE_TIMEOUT);

	return (ret < 0 ? ret : 0);
}
