/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/list.h>

/*
 * A Linux interrupt handler becomes an exec interrupt server on the
 * kernel IRQ. The threaded half, when the hard handler asks for it, is
 * queued to the high-priority work queue.
 */
struct irq_handler_entry {
    struct list_head node;
    unsigned int irq;
    irq_handler_t handler;
    irq_handler_t thread_fn;
    void *dev_id;
    const char *name;
    struct Interrupt is;
    struct work_struct thread_work;
};

static LIST_HEAD(irq_handlers);
static volatile unsigned long irq_count;

unsigned long nouveau_compat_irq_count(void)
{
    return irq_count;
}

static AROS_INTH1(irq_dispatcher, struct irq_handler_entry *, entry)
{
    AROS_INTFUNC_INIT

    irqreturn_t ret = entry->handler(entry->irq, entry->dev_id);

    if (ret != IRQ_NONE)
        irq_count++;

    if (ret == IRQ_WAKE_THREAD && entry->thread_fn)
        queue_work(system_highpri_wq, &entry->thread_work);

    return (ret != IRQ_NONE) ? TRUE : FALSE;

    AROS_INTFUNC_EXIT
}

static void irq_thread_work(struct work_struct *work)
{
    struct irq_handler_entry *entry = container_of(work, struct irq_handler_entry, thread_work);

    entry->thread_fn(entry->irq, entry->dev_id);
}

int request_threaded_irq(unsigned int irq, irq_handler_t handler, irq_handler_t thread_fn,
    unsigned long flags, const char *name, void *dev)
{
    struct irq_handler_entry *entry;

    if (!handler && !thread_fn)
        return -EINVAL;

    entry = kzalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry)
        return -ENOMEM;

    entry->irq = irq;
    entry->handler = handler;
    entry->thread_fn = thread_fn;
    entry->dev_id = dev;
    entry->name = name;
    INIT_WORK(&entry->thread_work, irq_thread_work);

    entry->is.is_Node.ln_Type = NT_INTERRUPT;
    entry->is.is_Node.ln_Pri = 10;
    entry->is.is_Node.ln_Name = (STRPTR)(name ? name : "nouveau");
    entry->is.is_Code = (VOID_FUNC)irq_dispatcher;
    entry->is.is_Data = entry;

    printk(KERN_NOTICE "[nouveau] request_irq: %s on IRQ %u\n", name, irq);

    list_add(&entry->node, &irq_handlers);
    AddIntServer(INTB_KERNEL + irq, &entry->is);
    return 0;
}

void *free_irq(unsigned int irq, void *dev_id)
{
    struct irq_handler_entry *entry, *tmp;

    list_for_each_entry_safe(entry, tmp, &irq_handlers, node) {
        if (entry->irq == irq && entry->dev_id == dev_id) {
            RemIntServer(INTB_KERNEL + irq, &entry->is);
            list_del(&entry->node);
            kfree(entry);
            return NULL;
        }
    }
    return NULL;
}
