#ifndef KERNEL_IRQAFFINITY_H
#define KERNEL_IRQAFFINITY_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Which CPU a KERNELTAG_IRQ_AFFINITY request means.
*/

#include <aros/kernel.h>
#include <exec/tasks.h>
#include <proto/kernel.h>

/*
 * An interrupt is delivered to one CPU. KERNELTAG_IRQ_AFFINITY names the
 * CPUs it may be delivered to, in the same form a task's affinity takes:
 *
 *   TASKAFFINITY_BOOT - the boot CPU. This is where every device interrupt
 *                       goes unless something asks for another CPU.
 *   TASKAFFINITY_ANY  - any CPU.
 *   a CPU mask        - from KrnAllocCPUMask()/KrnGetCPUMask().
 *
 * Of the CPUs named, the one with the lowest load (KATTR_CPULoad) at the
 * time of the call is taken; the lowest numbered of them if several are
 * equal. The choice is made once: the interrupt stays on that CPU until
 * its affinity is set again.
 *
 * The port says which CPUs there are to choose from: CPU numbers below
 * 'cpus' for which usable() is true. A CPU that never started has no
 * load, and would be the first choice if it were asked.
 *
 * Loads are compared in steps of a thirty-second of a CPU: the figures
 * of CPUs with nothing much to do differ by noise, and the lowest of them
 * means nothing. CPUs often carry the same load, then: when the system
 * starts none has much, and that is when most drivers ask. Of equally loaded CPUs
 * the boot CPU is the last choice, because it takes every interrupt
 * nobody asked about and runs the soft interrupts; of the others, the one
 * that has been given the fewest interrupts. 'given' is the port's count
 * of the interrupts each CPU has been given, or NULL.
 *
 * Returns the CPU number, or -1 if the request names no CPU that can take
 * interrupts. Call from task context, with KernelBase in scope.
 */
#define IRQAFFINITY_LOADSHIFT    27

static inline int krnIRQAffinityCPUGiven(struct KernelBase *KernelBase, IPTR affinity,
    unsigned int cpus, int (*usable)(struct KernelBase *, unsigned int),
    const unsigned int *given)
{
#if defined(__AROSEXEC_SMP__)
    unsigned int cpu;
    intptr_t load, best = 0;
    int target = -1;

    if (affinity == TASKAFFINITY_BOOT)
        return 0;
    if (affinity == TASKAFFINITY_ALL_BUT_SELF)
        return -1;

    for (cpu = 0; cpu < cpus; cpu++)
    {
        if ((affinity != TASKAFFINITY_ANY) && !KrnCPUInMask(cpu, (void *)affinity))
            continue;
        if (!usable(KernelBase, cpu))
            continue;

        /* KATTR_CPULoad is a fraction of a CPU scaled to 32 bits */
        load = (intptr_t)(((ULONG)KrnGetCPUAttr(KATTR_CPULoad, cpu)) >> IRQAFFINITY_LOADSHIFT);
        if ((target < 0) || (load < best) ||
            ((load == best) &&
             ((target == 0) || (given && (given[cpu] < given[target])))))
        {
            target = cpu;
            best = load;
        }
    }
    return target;
#else
    if ((affinity == TASKAFFINITY_BOOT) || (affinity == TASKAFFINITY_ANY))
        return 0;
    if (affinity == TASKAFFINITY_ALL_BUT_SELF)
        return -1;
    return KrnCPUInMask(0, (void *)affinity) ? 0 : -1;
#endif
}

/* The same, for a port that does not count the interrupts it has given */
static inline int krnIRQAffinityCPU(struct KernelBase *KernelBase, IPTR affinity,
    unsigned int cpus, int (*usable)(struct KernelBase *, unsigned int))
{
    return krnIRQAffinityCPUGiven(KernelBase, affinity, cpus, usable, NULL);
}

#endif /* !KERNEL_IRQAFFINITY_H */
