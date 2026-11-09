/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/dma-fence.h>
#include <linux/dma-fence-array.h>
#include <linux/dma-fence-chain.h>
#include <linux/dma-fence-unwrap.h>

/*
 * Fence chains are how a timeline sync object stores its points: each
 * node wraps the fence of one point and refers to the node before it. A
 * node counts as signalled once its own fence and all earlier nodes are,
 * and a walk along the chain drops nodes that have signalled, so a chain
 * only ever holds the points still outstanding.
 */

static struct dma_fence *chain_get_prev(struct dma_fence_chain *chain)
{
    struct dma_fence *prev;

    rcu_read_lock();
    prev = dma_fence_get_rcu_safe(&chain->prev);
    rcu_read_unlock();
    return prev;
}

/*
 * The next node of the chain, with the reference on this one dropped.
 * Signalled predecessors are unlinked on the way: the node is made to
 * point past them.
 */
struct dma_fence *dma_fence_chain_walk(struct dma_fence *fence)
{
    struct dma_fence_chain *chain, *prev_chain;
    struct dma_fence *prev, *replacement, *tmp;

    chain = to_dma_fence_chain(fence);
    if (!chain) {
        dma_fence_put(fence);
        return NULL;
    }

    while ((prev = chain_get_prev(chain))) {
        prev_chain = to_dma_fence_chain(prev);
        if (prev_chain) {
            if (!dma_fence_is_signaled(prev_chain->fence))
                break;
            replacement = chain_get_prev(prev_chain);
        } else {
            if (!dma_fence_is_signaled(prev))
                break;
            replacement = NULL;
        }

        tmp = cmpxchg(&chain->prev, prev, replacement);
        if (tmp == prev)
            dma_fence_put(tmp);
        else
            dma_fence_put(replacement);
        dma_fence_put(prev);
    }

    dma_fence_put(fence);
    return prev;
}

/*
 * Replaces *pfence (a chain head) with the node that stands for seqno:
 * the oldest node whose point is at least seqno. 0 means "whatever is
 * there", -EINVAL that the point does not exist on the chain yet.
 */
int dma_fence_chain_find_seqno(struct dma_fence **pfence, uint64_t seqno)
{
    struct dma_fence_chain *chain;

    if (!seqno)
        return 0;

    chain = to_dma_fence_chain(*pfence);
    if (!chain || chain->base.seqno < seqno)
        return -EINVAL;

    dma_fence_chain_for_each(*pfence, &chain->base) {
        if ((*pfence)->context != chain->base.context)
            break;
        if (to_dma_fence_chain(*pfence)->prev_seqno < seqno)
            break;
    }
    dma_fence_put(&chain->base);

    return 0;
}

static const char *chain_get_driver_name(struct dma_fence *fence)
{
    return "dma_fence_chain";
}

static const char *chain_get_timeline_name(struct dma_fence *fence)
{
    return "unbound";
}

static void chain_irq_work(struct irq_work *work)
{
    struct dma_fence_chain *chain = container_of(work, struct dma_fence_chain, work);

    /* try to hook into the next outstanding fence; none left means signalled */
    if (!dma_fence_chain_ops.enable_signaling(&chain->base))
        dma_fence_signal(&chain->base);
    dma_fence_put(&chain->base);
}

static void chain_cb(struct dma_fence *f, struct dma_fence_cb *cb)
{
    struct dma_fence_chain *chain = container_of(cb, struct dma_fence_chain, cb);

    init_irq_work(&chain->work, chain_irq_work);
    irq_work_queue(&chain->work);
    dma_fence_put(f);
}

static bool chain_enable_signaling(struct dma_fence *fence)
{
    struct dma_fence_chain *head = to_dma_fence_chain(fence);

    dma_fence_get(&head->base);
    dma_fence_chain_for_each(fence, &head->base) {
        struct dma_fence *f = dma_fence_chain_contained(fence);

        dma_fence_get(f);
        if (!dma_fence_add_callback(f, &head->cb, chain_cb)) {
            dma_fence_put(fence);
            return true;
        }
        dma_fence_put(f);
    }
    dma_fence_put(&head->base);

    return false;
}

static bool chain_signaled(struct dma_fence *fence)
{
    dma_fence_chain_for_each(fence, fence) {
        struct dma_fence *f = dma_fence_chain_contained(fence);

        if (!dma_fence_is_signaled(f)) {
            dma_fence_put(fence);
            return false;
        }
    }

    return true;
}

static void chain_release(struct dma_fence *fence)
{
    struct dma_fence_chain *chain = to_dma_fence_chain(fence);
    struct dma_fence *prev;

    /*
     * Earlier nodes that only this one still holds are unhooked one at
     * a time: dropping them through nested releases would recurse once
     * per node.
     */
    while ((prev = rcu_dereference_protected(chain->prev, true))) {
        struct dma_fence_chain *prev_chain;

        if (kref_read(&prev->refcount) > 1)
            break;

        prev_chain = to_dma_fence_chain(prev);
        if (!prev_chain)
            break;

        rcu_assign_pointer(chain->prev, rcu_dereference_protected(prev_chain->prev, true));
        rcu_assign_pointer(prev_chain->prev, NULL);
        dma_fence_put(prev);
    }
    dma_fence_put(prev);

    dma_fence_put(chain->fence);
    dma_fence_free(fence);
}

static void chain_set_deadline(struct dma_fence *fence, ktime_t deadline)
{
    dma_fence_chain_for_each(fence, fence) {
        struct dma_fence *f = dma_fence_chain_contained(fence);

        dma_fence_set_deadline(f, deadline);
    }
}

const struct dma_fence_ops dma_fence_chain_ops = {
    .use_64bit_seqno = true,
    .get_driver_name = chain_get_driver_name,
    .get_timeline_name = chain_get_timeline_name,
    .enable_signaling = chain_enable_signaling,
    .signaled = chain_signaled,
    .release = chain_release,
    .set_deadline = chain_set_deadline,
};

bool dma_fence_is_chain(struct dma_fence *fence)
{
    return fence->ops == &dma_fence_chain_ops;
}

/*
 * Makes chain the node for point seqno after prev, taking over the
 * references on prev and fence. The node joins prev's timeline when the
 * point is later than prev's; otherwise it starts a timeline of its own.
 */
void dma_fence_chain_init(struct dma_fence_chain *chain, struct dma_fence *prev,
    struct dma_fence *fence, uint64_t seqno)
{
    struct dma_fence_chain *prev_chain = to_dma_fence_chain(prev);
    uint64_t context;

    spin_lock_init(&chain->lock);
    rcu_assign_pointer(chain->prev, prev);
    chain->fence = fence;
    chain->prev_seqno = 0;
    init_irq_work(&chain->work, chain_irq_work);

    if (prev_chain && __dma_fence_is_later(seqno, prev->seqno, prev->ops)) {
        context = prev->context;
        chain->prev_seqno = prev->seqno;
    } else {
        context = dma_fence_context_alloc(1);
        if (prev_chain)
            seqno = max(prev->seqno, seqno);
    }

    dma_fence_init(&chain->base, &dma_fence_chain_ops, &chain->lock, context, seqno);
}

/* --- unwrapping ---------------------------------------------------------- */

static struct dma_fence *unwrap_array(struct dma_fence_unwrap *cursor)
{
    cursor->array = dma_fence_chain_contained(cursor->chain);
    cursor->index = 0;
    return dma_fence_array_first(cursor->array);
}

struct dma_fence *dma_fence_unwrap_first(struct dma_fence *head, struct dma_fence_unwrap *cursor)
{
    cursor->chain = dma_fence_get(head);
    return unwrap_array(cursor);
}

struct dma_fence *dma_fence_unwrap_next(struct dma_fence_unwrap *cursor)
{
    struct dma_fence *tmp;

    ++cursor->index;
    tmp = dma_fence_array_next(cursor->array, cursor->index);
    if (tmp)
        return tmp;

    cursor->chain = dma_fence_chain_walk(cursor->chain);
    return unwrap_array(cursor);
}

/*
 * One fence that signals once every plain fence behind the given ones
 * has: the single outstanding fence itself, an array of them, or the
 * signalled stub when nothing is outstanding. Fences of one context are
 * ordered, so only the latest of each is kept.
 */
struct dma_fence *__dma_fence_unwrap_merge(unsigned int num_fences, struct dma_fence **fences,
    struct dma_fence_unwrap *cursors)
{
    struct dma_fence_array *result;
    struct dma_fence **array, *tmp, *fence;
    unsigned int i, count = 0, kept = 0;

    for (i = 0; i < num_fences; i++)
        dma_fence_unwrap_for_each(tmp, &cursors[i], fences[i])
            count++;

    if (count == 0)
        return dma_fence_get_stub();

    array = kmalloc_array(count, sizeof(*array), GFP_KERNEL);
    if (!array)
        return NULL;

    for (i = 0; i < num_fences; i++) {
        dma_fence_unwrap_for_each(tmp, &cursors[i], fences[i]) {
            unsigned int j;

            if (dma_fence_is_signaled(tmp))
                continue;

            for (j = 0; j < kept; j++) {
                if (array[j]->context != tmp->context)
                    continue;
                if (dma_fence_is_later(tmp, array[j])) {
                    dma_fence_put(array[j]);
                    array[j] = dma_fence_get(tmp);
                }
                break;
            }
            if (j == kept)
                array[kept++] = dma_fence_get(tmp);
        }
    }

    if (kept == 0) {
        kfree(array);
        return dma_fence_get_stub();
    }

    if (kept == 1) {
        fence = array[0];
        kfree(array);
        return fence;
    }

    result = dma_fence_array_create(kept, array, dma_fence_context_alloc(1), 1, false);
    if (!result) {
        for (i = 0; i < kept; i++)
            dma_fence_put(array[i]);
        kfree(array);
        return NULL;
    }

    return &result->base;
}
