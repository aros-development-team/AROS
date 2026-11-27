/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Hardware interrupt plumbing for the sbi-riscv64 target.

    kernel.resource's generic KrnAddIRQHandler() keeps the handler
    chains; what it needs from the port is a way to unmask a source and
    a way to run the chain once one fires. Both go through the PLIC.
*/

#include <inttypes.h>

#include <asm/cpu.h>

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_interrupts.h>

#include "kernel_intern.h"

#if defined(__AROSEXEC_SMP__)
#include <aros/types/spinlock_s.h>
#include <utility/hooks.h>

extern spinlock_t *Kernel_52_KrnSpinLock(spinlock_t *, struct Hook *, ULONG, void *);
extern void Kernel_53_KrnSpinUnLock(spinlock_t *, void *);
#endif

/*
 * The trap handler runs with no library bases to hand, so the one
 * passed in when a source is first unmasked is kept for it. Nothing
 * can fire before that happens - an unmasked source is the only kind
 * the controller will report. There is only one KernelBase, so every
 * CPU that unmasks a source stores the same value here.
 */
static struct KernelBase *irqKernelBase;

/* Per-source delivery counts, for the serial debug poke: a wedged
   device shows up as a counter that stops moving. Counted by whichever
   hart took the interrupt, so on SMP the increment is atomic. */
ULONG __irq_counts[KRN_MAX_IRQ_SOURCES];

/*
 * Sources krnHandleExternalIRQ() throttled for storming, on whichever
 * CPU they are delivered to; the boot hart's timer tick re-enables them
 * (krnIRQUnthrottle), the controller's enable bits being reachable from
 * any hart. Masking one for good
 * silently killed the Titan's Ethernet: a level line a gigabit NIC
 * re-asserts before every completion looks exactly like a stuck one.
 *
 * A source is marked here only while it is masked for that reason and
 * still wanted. One whose last handler has gone is masked for good by
 * ictl_disable_irq() and must not come back on the next tick, so the
 * mark is taken off there, and it is only ever put on a source found
 * unmasked. On SMP the mark and the masking it goes with change
 * together under the lock that covers the handler chains: the two
 * interrupt paths (throttle and tick) hold it for reading, which lets
 * them run beside each other and beside krnRunIRQHandlers() on other
 * CPUs, and KrnAddIRQHandler()/KrnRemIRQHandler() hold it for writing
 * around the ictl_ calls. Uniprocessor builds need none: those callers
 * run with interrupts disabled.
 */
static volatile UBYTE __irq_throttled[KRN_MAX_IRQ_SOURCES];

/*
 * External interrupts in a row that a CPU took and found nothing to
 * claim for. One now and then is normal - the source was masked or
 * given to another CPU after it raised the line. An unbroken run means
 * the controller is signalling this hart through a context that is not
 * the one recorded for it, and the hart would do nothing else again.
 */
#define IRQ_MAX_UNCLAIMED       65536
static ULONG __irq_unclaimed[RISCV_MAXCPUS];

#if defined(__AROSEXEC_SMP__)
/* Only ever called from interrupt context, with interrupts off. kb is
   set before any source can be unmasked, let alone throttled. */
static inline void irq_chains_lock(struct KernelBase *kb)
{
    if (kb)
        Kernel_52_KrnSpinLock(&kb->kb_IntrSpinLock, NULL, SPINLOCK_MODE_READ, NULL);
}

static inline void irq_chains_unlock(struct KernelBase *kb)
{
    if (kb)
        Kernel_53_KrnSpinUnLock(&kb->kb_IntrSpinLock, NULL);
}
#else
#define irq_chains_lock(kb)     do { (void)(kb); } while (0)
#define irq_chains_unlock(kb)   do { (void)(kb); } while (0)
#endif

void krnIRQUnthrottle(void)
{
    struct KernelBase *kb = irqKernelBase;
    unsigned int n;

    for (n = 1; n < KRN_MAX_IRQ_SOURCES; n++)
    {
        /* Nearly always nothing is throttled: look before locking */
        if (!__irq_throttled[n])
            continue;

        irq_chains_lock(kb);
        /* Still marked? Its last handler may just have been removed */
        if (__irq_throttled[n])
        {
            __irq_throttled[n] = 0;
            krnPLICEnable(n, 1);
        }
        irq_chains_unlock(kb);
    }
}

/*
 * Sources above the controller's own are raised by whoever owns them
 * (see allocirq.c) and have nothing to unmask - a bridge receiving
 * message interrupts calls KrnRunIRQ() for them once its own line has
 * been serviced. Handing one to the controller is not an error, it
 * simply is not its source, so say nothing and leave it alone.
 */
void ictl_enable_irq(irqid_t irq, struct KernelBase *kb)
{
    irqKernelBase = kb;
#if defined(__AROSEXEC_SMP__)
    /* Visible to the other harts before the source can interrupt one */
    asm volatile("fence rw, rw" ::: "memory");
#endif
    /* Unmasked from here on, so no longer waiting for the tick */
    if (irq < KRN_MAX_IRQ_SOURCES)
        __irq_throttled[irq] = 0;
    if (irq <= krnPLICSourceCount())
        krnPLICEnable(irq, 1);
}

/*
 * The last handler of the source has gone (KrnRemIRQHandler(), which
 * holds the chain lock for writing on SMP and has interrupts disabled).
 * If it was throttled it stops being so here: masked for good is what
 * it has to be now, and the tick would unmask it.
 */
void ictl_disable_irq(irqid_t irq, struct KernelBase *kb)
{
    if (irq < KRN_MAX_IRQ_SOURCES)
        __irq_throttled[irq] = 0;
    if (irq <= krnPLICSourceCount())
        krnPLICEnable(irq, 0);
}

/*
 * Called from the trap handler on a supervisor external interrupt.
 * More than one source can be waiting, and each has to be completed
 * or the controller will not raise it again.
 */
void krnHandleExternalIRQ(void)
{
    /*
     * Legacy PCI interrupts are level triggered: the source stays
     * asserted until a handler quiets the device. One that nothing
     * services would spin here forever and the machine would simply
     * stop, with no clue why. Bound the loop, and mask a source that
     * will not go quiet so the rest of the system keeps running and
     * says which one it was.
     */
    unsigned int src, last = 0, spins = 0, claimed = 0;
    unsigned int cpu = krnRVCPUNumber();
    /* The context each claim came from: this hart's own business until
       the completion, so it is kept here and not in the controller */
    int ctx = -1;

    while ((src = krnPLICClaim(&ctx)) != 0)
    {
        struct KernelBase *kb = irqKernelBase;

        claimed++;
        if (src < KRN_MAX_IRQ_SOURCES)
        {
#if defined(__AROSEXEC_SMP__)
            __atomic_fetch_add(&__irq_counts[src], 1, __ATOMIC_RELAXED);
#else
            __irq_counts[src]++;
#endif
        }

        if (kb)
        {
            struct krnMSIController mc;

            if (krnGetMSIController(kb, src, &mc))
            {
                /*
                 * A controller collecting messages: nothing is wired
                 * to this source but itself, and its pending register
                 * is the only thing that says which devices signalled.
                 * Clear what has arrived, then run the handlers of the
                 * sources those vectors were given. Its description
                 * is a copy (see krnGetMSIController); the register
                 * itself needs no lock: a source is delivered to one
                 * CPU, and the controller does not forward it again
                 * before it is completed, even once given to another.
                 */
                ULONG pending = *(volatile ULONG *)mc.status;

                if (pending)
                {
                    *(volatile ULONG *)mc.status = pending;
                    (void)*(volatile ULONG *)mc.status;

                    while (pending)
                    {
                        unsigned int vector = __builtin_ctz(pending);

                        pending &= ~(1UL << vector);
                        if (vector < mc.count)
                            krnRunIRQHandlers(kb,
                                              (irqid_t)(mc.base + vector));
                    }
                }
            }
            else
                krnRunIRQHandlers(kb, (irqid_t)src);
        }
        krnPLICComplete(ctx, src);

        /*
         * Only one source coming back over and over is a stuck line.
         * A busy machine can legitimately deliver a long run here, so
         * count repeats of the same source rather than the length of
         * the run, and give it room to be merely busy.
         */
        spins = (src == last) ? spins + 1 : 0;
        last = src;

        if (spins > 65536)
        {
            int was;

            /*
             * Throttle, never mask for good: the next tick re-enables
             * it. Unless it is masked already - then its last handler
             * was removed while it stormed (or another hart got here
             * first), and it is not this path's to bring back.
             */
            irq_chains_lock(kb);
            was = krnPLICEnable(src, 0);
            if (was && src < KRN_MAX_IRQ_SOURCES)
                __irq_throttled[src] = 1;
            irq_chains_unlock(kb);

            if (was)
            {
                krnSBIPutStr("[irq] source ");
                krnSBIPutDec(src);
                krnSBIPutStr(" storming - throttled until the next tick\n");
            }
            break;
        }
    }

    if (cpu < RISCV_MAXCPUS)
    {
        if (claimed)
            __irq_unclaimed[cpu] = 0;
        else if (++__irq_unclaimed[cpu] > IRQ_MAX_UNCLAIMED)
        {
            /* Better without device interrupts than without the CPU */
            csr_clear(sie, SIE_SEIE);
            __irq_unclaimed[cpu] = 0;
            krnSBIPutStr("[irq] cpu ");
            krnSBIPutDec(cpu);
            krnSBIPutStr(": external interrupts with nothing to claim -"
                         " its PLIC context is not the one recorded;"
                         " closed to them\n");
        }
    }
}
