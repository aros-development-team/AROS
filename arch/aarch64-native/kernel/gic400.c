/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: GIC-400 (GICv2) per-core setup, SGI IPIs and the routing of shared
    interrupts for the Pi 4 and Pi 5, which lack the Pi 3's BCM2836 IPI
    mailboxes. SGIs are IRQs here: group 0 is secure-only, and Disable()
    masks I and F alike anyway.
*/

#include <aros/kernel.h>

#include "kernel_base.h"

#include <proto/kernel.h>
#include <proto/exec.h>

#include <inttypes.h>

#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_cpu.h"
#include "kernel_interrupts.h"
#include "io.h"

#include "exec_platform.h"

#include "gic400.h"

#define GICD(o)     (*(volatile uint32_t *)(gic_dist + (o)))
#define GICC(o)     (*(volatile uint32_t *)(gic_cpuif + (o)))

#define GICD_ISENABLER  0x100
#define GICD_IPRIORITYR 0x400
#define GICD_ITARGETSR  0x800
#define GICD_SGIR       0xF00

/* INTIDs below this are private to a core (SGIs and PPIs) */
#define GIC400_FIRST_SPI    32

#define GICC_CTLR   0x000
#define GICC_PMR    0x004

#define GIC400_MAXCPUS  4

static uintptr_t gic_dist;
static uintptr_t gic_cpuif;

void gic400_setbase(uintptr_t dist, uintptr_t cpuif)
{
    gic_dist = dist;
    gic_cpuif = cpuif;
}

#if defined(__AROSEXEC_SMP__)
extern void handle_ipi(uint32_t, uint32_t);

/* SGIs carry no payload: one slot per (target, source) so two senders
 * can't clobber each other's data word. */
struct gic_ipislot
{
    volatile uint32_t   is_Msg;
    volatile uint32_t   is_Data;
};

static struct gic_ipislot gic_ipi_slot[GIC400_MAXCPUS][GIC400_MAXCPUS];
#endif

/* PMR, GICC_CTLR and the first ISENABLER are banked per core */
void gic400_init_core(void)
{
    GICC(GICC_PMR) = 0xF0;
    GICC(GICC_CTLR) = 1;

#if defined(__AROSEXEC_SMP__)
    *((volatile uint8_t *)(gic_dist + GICD_IPRIORITYR + GIC400_IPI_SGI)) = 0xA0;
    GICD(GICD_ISENABLER) = 1u << GIC400_IPI_SGI;
#endif
}

/*
 * The core each shared interrupt is delivered to; 0, the boot core, until
 * something asks for another. Kept here and not read back from the
 * distributor, whose target registers hold whatever the firmware left.
 *
 * Written under kb_IntrSpinLock in write mode, which is also what
 * KrnAddIRQHandler() holds around the enable: a new target is either
 * seen by the enable or written after it, never lost between the two.
 */
static uint8_t gic_irq_cpu[IRQ_COUNT];

#define GICD_TARGET(irq) \
    (*(volatile uint8_t *)(gic_dist + GICD_ITARGETSR + (irq)))

/*
 * Point a shared interrupt at its core. The target registers hold a byte
 * for each interrupt and that byte is written whole, so interrupts next
 * to this one are not touched.
 */
void gic400_irq_route(int irq)
{
    if ((irq >= GIC400_FIRST_SPI) && (irq < IRQ_COUNT))
        GICD_TARGET(irq) = 1u << gic_irq_cpu[irq];
}

/*
 * Deliver a shared interrupt to another core. Returns 0 if it can not be:
 * a private interrupt (SGI, PPI) belongs to every core by nature, and the
 * distributor ignores the write for an interrupt or a core it does not
 * have, or one this (non-secure) side may not change.
 *
 * Nothing is masked for the change. An interrupt that is pending moves to
 * the new core with the write. One that a core has acknowledged stays
 * active on that core and is ended there, by the same pass of the dispatch
 * loop that read it. That does not keep it from the new core: its line is
 * still up until a handler has seen to the device, and the distributor
 * signals it to the new core at once (active is counted for each core in
 * QEMU, and a GIC-400 may do the same for one that is active and pending).
 * gic400_run_irq() is what keeps the handlers on one core.
 */
int gic400_irq_settarget(int irq, int cpu)
{
    uint8_t target;

    if ((irq < GIC400_FIRST_SPI) || (irq >= IRQ_COUNT))
        return 0;
    if ((cpu < 0) || (cpu >= GIC400_MAXCPUS))
        return 0;

    target = 1u << cpu;

    GICD_TARGET(irq) = target;
    if (GICD_TARGET(irq) != target)
    {
        /* Not taken: leave it where it was */
        GICD_TARGET(irq) = 1u << gic_irq_cpu[irq];
        return 0;
    }

    gic_irq_cpu[irq] = cpu;

    return 1;
}

/* The core a shared interrupt is delivered to; -1 for a private one */
int gic400_irq_gettarget(int irq)
{
    if ((irq < GIC400_FIRST_SPI) || (irq >= IRQ_COUNT))
        return -1;

    return gic_irq_cpu[irq];
}

/*
 * Run the handlers of an interrupt this core has acknowledged, if the
 * interrupt is this core's.
 *
 * A shared interrupt can be given to another core between the acknowledge
 * and here: KrnModifyIRQA() only waits for the cores that are inside
 * krnRunIRQHandlers(). The new core is then signalled the same interrupt
 * and both would run its handlers, at the same time. So the core is
 * compared with the target under kb_IntrSpinLock in read mode, which
 * KrnModifyIRQA() takes in write mode to change the target: the target
 * stays what it is while any core runs handlers, only the core it names
 * runs them, and that makes it one core at a time. krnRunIRQHandlers()
 * takes the lock in read mode again, which a reader may do: a writer that
 * waits does not hold readers off.
 *
 * The core that finds the interrupt gone does nothing but end it. Nothing
 * is lost by that: every shared interrupt is level triggered here (see the
 * enable), so its line stays up and the core it was given to takes it.
 */
void gic400_run_irq(struct KernelBase *KernelBase, int irq)
{
#if defined(__AROSEXEC_SMP__)
    KrnSpinLock(&KernelBase->kb_IntrSpinLock, NULL, SPINLOCK_MODE_READ);
    if ((irq < GIC400_FIRST_SPI) || (irq >= IRQ_COUNT) ||
        (gic_irq_cpu[irq] == GetCPUNumber()))
    {
        krnRunIRQHandlers(KernelBase, irq);
    }
    KrnSpinUnLock(&KernelBase->kb_IntrSpinLock);
#else
    krnRunIRQHandlers(KernelBase, irq);
#endif
}

void gic400_send_ipi(uint32_t ipi, uint32_t ipi_data, uint32_t cpumask)
{
#if defined(__AROSEXEC_SMP__)
    int src = GetCPUNumber();
    int cpu;

    /* Disable() keeps a Signal from re-entering. */
    Disable();

    for (cpu = 0; cpu < GIC400_MAXCPUS; cpu++)
    {
        if (!(cpumask & (1 << cpu)))
            continue;

        gic_ipi_slot[cpu][src].is_Data = ipi_data;
        __atomic_fetch_or(&gic_ipi_slot[cpu][src].is_Msg, ipi, __ATOMIC_RELEASE);
    }

    /* Order the Normal-memory slots before the Device SGIR write */
    dsb();

    /* Bits 23:16 are the CPU target list */
    GICD(GICD_SGIR) = ((cpumask & 0xff) << 16) | GIC400_IPI_SGI;

    Enable();
#else
    (void)ipi; (void)ipi_data; (void)cpumask;
#endif
}

void gic400_handle_ipi(void)
{
#if defined(__AROSEXEC_SMP__)
    int cpu = GetCPUNumber();
    int src;

    for (src = 0; src < GIC400_MAXCPUS; src++)
    {
        /* Bits set after the exchange arrive with a fresh SGI */
        uint32_t msg = __atomic_exchange_n(&gic_ipi_slot[cpu][src].is_Msg, 0,
                                           __ATOMIC_ACQUIRE);

        if (msg)
            handle_ipi(msg, gic_ipi_slot[cpu][src].is_Data);
    }
#endif
}
