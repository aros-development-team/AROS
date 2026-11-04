/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/dma-fence.h>

#include <drm-compat/drm_compat_funcs.h>

/*
 * A fence whose signalling callbacks must fire even if the completion
 * interrupt never arrives. Submitted fences are looked at from a work item
 * until they signal: dma_fence_is_signaled() asks the fence's own check,
 * which sees the sequence the engine wrote and signals the fence, running
 * its callbacks. Waiters polling the fence themselves do not need this; the
 * scheduler, which only ever listens for the callback, does.
 */

struct fence_poll {
    struct list_head node;
    struct dma_fence *fence;
};

static LINUX_LIST_HEAD(poll_list);
static DEFINE_SPINLOCK(poll_lock);
static struct delayed_work poll_work;
static bool poll_armed;
static bool poll_initialised;

#define FENCE_POLL_INTERVAL     1   /* jiffies */

static void fence_poll_fn(struct work_struct *work)
{
    struct fence_poll *p, *tmp;
    bool more;

    spin_lock(&poll_lock);
    list_for_each_entry_safe(p, tmp, &poll_list, node) {
        struct dma_fence *fence = p->fence;
        bool done;

        spin_unlock(&poll_lock);
        done = dma_fence_is_signaled(fence);
        spin_lock(&poll_lock);
        if (!done)
            continue;
        list_del(&p->node);
        dma_fence_put(fence);
        kfree(p);
    }
    more = !list_empty(&poll_list);
    poll_armed = more;
    spin_unlock(&poll_lock);

    if (more)
        queue_delayed_work(system_wq, &poll_work, FENCE_POLL_INTERVAL);
}

void dma_fence_poll(struct dma_fence *fence)
{
    struct fence_poll *p;
    bool arm;

    if (!fence || dma_fence_is_signaled(fence))
        return;

    p = kzalloc(sizeof(*p), GFP_KERNEL);
    if (!p)
        return;
    p->fence = dma_fence_get(fence);

    spin_lock(&poll_lock);
    if (!poll_initialised) {
        INIT_DELAYED_WORK(&poll_work, fence_poll_fn);
        poll_initialised = true;
    }
    list_add_tail(&p->node, &poll_list);
    arm = !poll_armed;
    poll_armed = true;
    spin_unlock(&poll_lock);

    if (arm)
        queue_delayed_work(system_wq, &poll_work, FENCE_POLL_INTERVAL);
}
