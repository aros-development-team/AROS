/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dostags.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/irqdomain.h>

#define COMPAT_VIRQ_BASE    0x10000

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
    struct Task *irq_task;          /* MSI: the handler runs in this task */
    ULONG irq_sigmask;
    volatile ULONG irq_pending;
    volatile BOOL irq_stop;
    struct Task *irq_parent;
    ULONG irq_ack;                  /* parent's signal for start/stop handshakes */
    volatile BOOL irq_done;
};

static LIST_HEAD(irq_handlers);
static volatile unsigned long irq_count;

unsigned long amdgpu_compat_irq_count(void)
{
    return irq_count;
}

/* MSI vectors, recorded by pci_enable_msi(); their handlers run in a task */
#define COMPAT_MAX_MSI 8
static unsigned int compat_msi_irqs[COMPAT_MAX_MSI];
static int compat_msi_count;

void compat_irq_mark_msi(unsigned int irq)
{
    if (compat_msi_count < COMPAT_MAX_MSI)
        compat_msi_irqs[compat_msi_count++] = irq;
}

static bool compat_irq_is_msi(unsigned int irq)
{
    int i;

    for (i = 0; i < compat_msi_count; i++)
        if (compat_msi_irqs[i] == irq)
            return true;
    return false;
}

static void irq_run_handler(struct irq_handler_entry *entry)
{
    irqreturn_t ret = entry->handler(entry->irq, entry->dev_id);

    if (ret != IRQ_NONE)
        irq_count++;
    if (ret == IRQ_WAKE_THREAD && entry->thread_fn)
        queue_work(system_highpri_wq, &entry->thread_work);
}

static void irq_task_main(void)
{
    struct irq_handler_entry *entry = FindTask(NULL)->tc_UserData;
    BYTE sigbit = AllocSignal(-1);

    entry->irq_sigmask = (sigbit >= 0) ? (1UL << sigbit) : 0;
    Signal(entry->irq_parent, entry->irq_ack);
    if (sigbit < 0)
        return;

    while (!entry->irq_stop)
    {
        Wait(entry->irq_sigmask);
        while (entry->irq_pending && !entry->irq_stop)
        {
            __atomic_sub_fetch(&entry->irq_pending, 1, __ATOMIC_ACQ_REL);
            irq_run_handler(entry);
        }
    }
    FreeSignal(sigbit);
    entry->irq_done = TRUE;
    Signal(entry->irq_parent, entry->irq_ack);
}

static AROS_INTH1(irq_dispatcher, struct irq_handler_entry *, entry)
{
    AROS_INTFUNC_INIT

    irqreturn_t ret;

    if (entry->irq_task)
    {
        __atomic_add_fetch(&entry->irq_pending, 1, __ATOMIC_ACQ_REL);
        Signal(entry->irq_task, entry->irq_sigmask);
        return TRUE;
    }

    ret = entry->handler(entry->irq, entry->dev_id);

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
    BYTE ack;

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
    entry->is.is_Node.ln_Name = (STRPTR)(name ? name : "amdgpu");
    entry->is.is_Code = (VOID_FUNC)irq_dispatcher;
    entry->is.is_Data = entry;

    if (irq < COMPAT_VIRQ_BASE && handler && compat_irq_is_msi(irq) && (ack = AllocSignal(-1)) >= 0)
    {
        entry->irq_ack = 1UL << ack;
        entry->irq_parent = FindTask(NULL);
        entry->irq_task = (struct Task *)CreateNewProcTags(
            NP_Name, (IPTR)"Amdgpu IRQ",
            NP_Priority, 30,
            NP_Affinity, TASKAFFINITY_ANY,
            NP_Entry, (IPTR)irq_task_main,
            NP_StackSize, 128 * 1024,
            NP_UserData, (IPTR)entry,
            TAG_DONE);
        if (entry->irq_task)
            Wait(entry->irq_ack);
        FreeSignal(ack);
        if (!entry->irq_task || !entry->irq_sigmask)
            entry->irq_task = NULL;
    }

    printk(KERN_INFO "[amdgpu] request_irq: %s on IRQ %u%s\n", name, irq,
           entry->irq_task ? " (MSI, handled in a task)" : "");

    list_add(&entry->node, &irq_handlers);
    if (irq < COMPAT_VIRQ_BASE)
        AddIntServer(INTB_KERNEL + irq, &entry->is);
    return 0;
}

void *free_irq(unsigned int irq, void *dev_id)
{
    struct irq_handler_entry *entry, *tmp;

    list_for_each_entry_safe(entry, tmp, &irq_handlers, node) {
        if (entry->irq == irq && entry->dev_id == dev_id) {
            if (irq < COMPAT_VIRQ_BASE)
                RemIntServer(INTB_KERNEL + irq, &entry->is);
            if (entry->irq_task)
            {
                BYTE ack = AllocSignal(-1);

                entry->irq_ack = (ack >= 0) ? (1UL << ack) : 0;
                entry->irq_parent = FindTask(NULL);
                entry->irq_stop = TRUE;
                Signal(entry->irq_task, entry->irq_sigmask);
                if (ack >= 0)
                {
                    Wait(entry->irq_ack);
                    FreeSignal(ack);
                }
                while (!entry->irq_done)
                    Delay(1);
            }
            list_del(&entry->node);
            kfree(entry);
            return NULL;
        }
    }
    return NULL;
}

struct irq_domain {
    const struct irq_domain_ops *ops;
    void *host_data;
    unsigned int size;
    unsigned int virq[];
};

static unsigned int next_virq = COMPAT_VIRQ_BASE;

struct irq_domain *irq_domain_create_linear(struct fwnode_handle *fwnode, unsigned int size,
                                            const struct irq_domain_ops *ops, void *host_data)
{
    struct irq_domain *d = kzalloc(sizeof(*d) + size * sizeof(d->virq[0]), GFP_KERNEL);

    if (!d)
        return NULL;
    d->ops = ops;
    d->host_data = host_data;
    d->size = size;
    return d;
}

void irq_domain_remove(struct irq_domain *d)
{
    unsigned int i;

    if (!d)
        return;
    for (i = 0; i < d->size; i++)
        if (d->virq[i] && d->ops && d->ops->unmap)
            d->ops->unmap(d, d->virq[i]);
    kfree(d);
}

unsigned int irq_find_mapping(struct irq_domain *d, irq_hw_number_t hwirq)
{
    return (d && hwirq < d->size) ? d->virq[hwirq] : 0;
}

unsigned int irq_create_mapping(struct irq_domain *d, irq_hw_number_t hwirq)
{
    unsigned int virq;

    if (!d || hwirq >= d->size)
        return 0;
    if (d->virq[hwirq])
        return d->virq[hwirq];
    virq = next_virq++;
    if (d->ops && d->ops->map && d->ops->map(d, virq, hwirq))
        return 0;
    d->virq[hwirq] = virq;
    return virq;
}

void irq_set_chip_and_handler(unsigned int irq, const struct irq_chip *chip, irq_flow_handler_t handle)
{
}

void handle_simple_irq(struct irq_desc *desc)
{
}

int generic_handle_domain_irq(struct irq_domain *d, unsigned int hwirq)
{
    struct irq_handler_entry *entry;
    unsigned int virq = irq_find_mapping(d, hwirq);
    int handled = 0;

    if (!virq)
        return -EINVAL;
    list_for_each_entry(entry, &irq_handlers, node) {
        if (entry->irq == virq && entry->handler) {
            if (entry->handler(virq, entry->dev_id) == IRQ_WAKE_THREAD && entry->thread_fn)
                queue_work(system_highpri_wq, &entry->thread_work);
            handled = 1;
        }
    }
    return handled ? 0 : -EINVAL;
}
