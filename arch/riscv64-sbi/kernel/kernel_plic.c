/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Platform level interrupt controller for the sbi-riscv64
          target.

    Every device interrupt on a RISC-V board arrives through the PLIC:
    the core itself only reports "an external interrupt happened", and
    which device it was has to be claimed from the controller. Until
    this is running a driver can do nothing but poll.

    The register layout is fixed by the PLIC specification:

        base + 0x000000 + 4*n           priority of source n
        base + 0x001000                 pending bits
        base + 0x002000 + 0x80*c        enable bits for context c
        base + 0x200000 + 0x1000*c      priority threshold for context c
        base + 0x200004 + 0x1000*c      claim / complete for context c

    A "context" is one hart in one privilege mode. Which one belongs to
    which hart is worked out from the device tree (see kernel_fdt.c).

    A source is delivered to one CPU: it is enabled in that CPU's
    supervisor context and in no other, and every hart claims from its
    own context only. That is the boot CPU unless KrnModifyIRQA() was
    asked for another (KERNELTAG_IRQ_AFFINITY).
*/

#include <inttypes.h>

#include <asm/cpu.h>
#include <asm/riscv64/mmu.h>

#include "kernel_intern.h"

#define PLIC_PRIORITY(n)        (0x000000 + 4 * (n))
#define PLIC_PENDING(n)         (0x001000 + 4 * ((n) / 32))
#define PLIC_ENABLE(c, n)       (0x002000 + 0x80 * (c) + 4 * ((n) / 32))
#define PLIC_THRESHOLD(c)       (0x200000 + 0x1000 * (c))
#define PLIC_CLAIM(c)           (0x200004 + 0x1000 * (c))

static unsigned long plic_base;
/* The boot hart's context as the firmware described it; set once at init */
static int           plic_context = -1;
static unsigned int  plic_ndev;
static uint32_t      plic_quirks;

/*
 * The enable words are read-modify-written by krnPLICEnable and
 * krnPLICSetTarget from any CPU, by a completion that finds its source
 * masked and, on a controller with the claim quirk, by the claim path
 * on the interrupt CPU; one tiny lock keeps those apart, and covers the
 * two tables below with them. Taken with interrupts off so the interrupt
 * path cannot interrupt its own holder, and nothing else is taken while
 * it is held.
 */
static volatile int  plic_lock;

static inline unsigned long plic_lock_acquire(void)
{
    unsigned long sstatus = csr_read(sstatus);

    csr_clear(sstatus, SSTATUS_SIE);
    while (__atomic_exchange_n(&plic_lock, 1, __ATOMIC_ACQUIRE))
        __asm__ __volatile__ ("pause" ::: "memory");
    return sstatus;
}

static inline void plic_lock_release(unsigned long sstatus)
{
    __atomic_store_n(&plic_lock, 0, __ATOMIC_RELEASE);
    if (sstatus & SSTATUS_SIE)
        csr_set(sstatus, SSTATUS_SIE);
}


unsigned int krnPLICSourceCount(void)
{
    return plic_ndev;
}

/* Every supervisor context the firmware described */
static int           plic_ctxlist[KRN_MAX_PLIC_CONTEXTS];
static unsigned int  plic_nctx;

/* The specification numbers sources 1 to 1023 */
#define PLIC_MAX_SOURCES        1024

/*
 * The supervisor context of each logical CPU, plus one; 0 for a CPU
 * that cannot be given a source. Each hart records its own once it is
 * able to take external interrupts (krnPLICInitCore), so an entry also
 * says the hart is running. Until the boot hart has recorded its own,
 * the context worked out for it at boot stands in.
 */
static int           plic_cpuctx[RISCV_MAXCPUS];

/* The CPU each source is delivered to. 0 is the boot CPU, which is
   where a source goes until it is given another target. */
static UBYTE         plic_irqcpu[PLIC_MAX_SOURCES];

static inline int plic_cpu_context(unsigned int cpu)
{
    if (cpu < RISCV_MAXCPUS && plic_cpuctx[cpu])
        return plic_cpuctx[cpu] - 1;
    return (cpu == 0) ? plic_context : -1;
}

static inline uint32_t plic_read(unsigned long off)
{
    return *(volatile uint32_t *)(plic_base + off);
}

static inline void plic_write(unsigned long off, uint32_t val)
{
    *(volatile uint32_t *)(plic_base + off) = val;
}

/*
 * Bring the controller up: every source masked and silenced, the
 * threshold dropped so that anything later enabled can get through.
 */
int krnPLICInit(struct krnFDTInfo *info)
{
    unsigned int n;

    if (!info->plic_base || info->plic_context < 0)
    {
        krnSBIPutStr("plic:      not described by the device tree -"
                     " devices will have to poll\n");
        return 0;
    }

    plic_base = (unsigned long)info->plic_base;
    plic_context = info->plic_context;
    plic_ndev = info->plic_ndev;
    plic_quirks = info->plic_quirks;
    if (plic_quirks & KRN_PLIC_QUIRK_CLAIM)
        krnSBIPutStr("plic:      UltraRISC claim erratum - sources are"
                     " isolated before each claim\n");

    plic_nctx = info->plic_ncontexts;
    for (n = 0; n < plic_nctx; n++)
        plic_ctxlist[n] = info->plic_contexts[n];

    /* A warm restart comes through here with the old kernel's targets */
    for (n = 0; n < RISCV_MAXCPUS; n++)
        plic_cpuctx[n] = 0;
    for (n = 0; n < PLIC_MAX_SOURCES; n++)
        plic_irqcpu[n] = 0;

    /* Nothing described? Fall back to the one worked out for the hart */
    if (!plic_nctx && plic_context >= 0)
    {
        plic_ctxlist[0] = plic_context;
        plic_nctx = 1;
    }

    /*
     * The MMU is already live by this point, and it only maps RAM, so
     * the controller has to be mapped before it can be touched.
     */
    if (!krnMMUMapPages(plic_base, plic_base,
                        info->plic_size ? (unsigned long)info->plic_size
                                        : 0x400000,
                        PTE_R | PTE_W))
    {
        krnSBIPutStr("plic:      could not map its registers!\n");
        plic_base = 0;
        return 0;
    }

    /* Source 0 does not exist; numbering starts at 1 */
    for (n = 1; n <= plic_ndev; n++)
    {
        unsigned int c;

        plic_write(PLIC_PRIORITY(n), 0);
        for (c = 0; c < plic_nctx; c++)
            plic_write(PLIC_ENABLE(plic_ctxlist[c], n),
                       plic_read(PLIC_ENABLE(plic_ctxlist[c], n)) &
                       ~(1U << (n & 31)));
    }

    /* Let anything with a non-zero priority through */
    for (n = 0; n < plic_nctx; n++)
        plic_write(PLIC_THRESHOLD(plic_ctxlist[n]), 0);

    /* Now that a source can be claimed, take external interrupts */
    csr_set(sie, SIE_SEIE);

    krnSBIPutStr("plic:      ");
    krnSBIPutHex(plic_base);
    krnSBIPutStr(", ");
    krnSBIPutDec(plic_ndev);
    krnSBIPutStr(" sources, ");
    krnSBIPutDec(plic_nctx);
    krnSBIPutStr(" supervisor context(s), assuming ");
    krnSBIPutDec((unsigned int)plic_context);
    krnSBIPutStr("\n");

    return 1;
}

/*
 * The calling hart can take external interrupts from here on, through
 * the supervisor context given: record it for the CPU, so that sources
 * can be targeted at it, and open the context and the hart to them. No
 * source is enabled in a secondary hart's context until one is given
 * that CPU as its target, so until then nothing arrives.
 *
 * On the boot hart this replaces the context worked out at boot with
 * the one the per-hart table holds (platform_init.c, which has checked
 * with the firmware which hart that is). It relies on no source having
 * been unmasked earlier: nothing can be before kernel.resource is up.
 *
 * A context another CPU already has is refused, and the hart is left
 * closed to external interrupts: two harts claiming from one context
 * would take each other's sources.
 */
void krnPLICInitCore(int ctx)
{
    unsigned int cpu = krnRVCPUNumber(), n;
    unsigned long sstatus;

    if (!plic_base || ctx < 0 || cpu >= RISCV_MAXCPUS)
        return;

    sstatus = plic_lock_acquire();
    for (n = 0; n < RISCV_MAXCPUS; n++)
    {
        if (n != cpu && plic_cpu_context(n) == ctx)
        {
            plic_lock_release(sstatus);
            return;
        }
    }
    plic_write(PLIC_THRESHOLD(ctx), 0);
    plic_cpuctx[cpu] = ctx + 1;
    plic_lock_release(sstatus);

    csr_set(sie, SIE_SEIE);
}

/*
 * Unmask a source and give it a usable priority, or mask it again, in
 * the context of the CPU it is delivered to. Priority 1 is enough - the
 * threshold is 0 and this kernel does not rank device interrupts
 * against each other.
 *
 * Returns whether the source was unmasked before the call, so a caller
 * masking it can tell that somebody else already had.
 */
int krnPLICEnable(unsigned int irq, int enable)
{
    uint32_t bits, bit = 1U << (irq & 31);
    int ctx, was = 0;

    if (!plic_base || !irq || irq > plic_ndev || irq >= PLIC_MAX_SOURCES)
    {
        krnSBIPutStr("[plic] refusing source ");
        krnSBIPutDec(irq);
        krnSBIPutStr(" (ndev ");
        krnSBIPutDec(plic_ndev);
        krnSBIPutStr(")\n");
        return 0;
    }

    unsigned long sstatus = plic_lock_acquire();

    /* A target always has a context: krnPLICSetTarget() takes no other */
    ctx = plic_cpu_context(plic_irqcpu[irq]);
    if (ctx >= 0)
    {
        bits = plic_read(PLIC_ENABLE(ctx, irq));
        was = (bits & bit) ? 1 : 0;
        if (enable)
            bits |= bit;
        else
            bits &= ~bit;
        plic_write(PLIC_ENABLE(ctx, irq), bits);
    }

    /*
     * Under the lock with the enable bits: two CPUs changing the same
     * source must not leave its priority saying one thing and its
     * enables the other. And after them, not before: a controller that
     * does not look at its outputs again when an enable word is written
     * (see krnPLICSetTarget) does when a priority is, and has to find
     * the enable bit as it now is. A source that was already pending
     * when it was unmasked would otherwise not be signalled.
     */
    plic_write(PLIC_PRIORITY(irq), enable ? 1 : 0);

    plic_lock_release(sstatus);

    return was;
}

/*
 * Deliver a source to another CPU. Returns 0, with nothing changed,
 * when that CPU has no context to take it in (not running, or the
 * firmware described none for its hart) or irq is not a source of the
 * controller.
 *
 * An unmasked source moves with its enable bit, masked in the old
 * context before it is unmasked in the new one so that it is never
 * offered to two harts. One that is pending and not yet claimed is
 * simply offered to the new hart instead; the old one, if it had
 * already trapped for it, finds nothing to claim. One the old hart has
 * claimed stays that hart's until its completion: the controller does
 * not forward a source again before that, so the new hart cannot be in
 * its handlers at the same time, and krnPLICComplete() completes in the
 * claiming context whether or not the source is still enabled there.
 */
int krnPLICSetTarget(unsigned int irq, unsigned int cpu)
{
    uint32_t bits, bit = 1U << (irq & 31);
    unsigned long sstatus;
    int oldctx, newctx;

    if (!plic_base || !irq || irq > plic_ndev || irq >= PLIC_MAX_SOURCES)
        return 0;

    sstatus = plic_lock_acquire();

    newctx = plic_cpu_context(cpu);
    if (newctx < 0)
    {
        plic_lock_release(sstatus);
        return 0;
    }

    oldctx = plic_cpu_context(plic_irqcpu[irq]);
    if (oldctx >= 0 && oldctx != newctx)
    {
        bits = plic_read(PLIC_ENABLE(oldctx, irq));
        if (bits & bit)
        {
            plic_write(PLIC_ENABLE(oldctx, irq), bits & ~bit);
            plic_write(PLIC_ENABLE(newctx, irq),
                       plic_read(PLIC_ENABLE(newctx, irq)) | bit);

            /*
             * A controller need not look at its outputs again because
             * an enable word was written. QEMU's does so when a source
             * rises, on a claim or a completion and on a priority or
             * threshold write, and at no other time: a source already
             * pending, or one that rose or that its old hart tried to
             * claim between the two writes above, is left enabled in the
             * new context and pending, its hart never signalled - and the
             * device, waiting to be serviced, raises nothing more.
             * Writing the priority back makes such a controller look;
             * to any other it is a write that changes nothing. The
             * value cannot change in between: krnPLICEnable() is its
             * only writer, under this lock.
             */
            plic_write(PLIC_PRIORITY(irq), plic_read(PLIC_PRIORITY(irq)));
        }
    }
    plic_irqcpu[irq] = (UBYTE)cpu;

    plic_lock_release(sstatus);

    return 1;
}

/* The CPU a source is delivered to, or -1 when it is not a source of
   the controller */
/*
 * How many sources each CPU below 'cpus' has been given. Sources nobody
 * asked about are on the boot CPU and are not counted for it: the boot
 * CPU is the last choice among equals anyway.
 */
void krnPLICCountTargets(unsigned int *given, unsigned int cpus)
{
    unsigned int irq, cpu;

    for (cpu = 0; cpu < cpus; cpu++)
        given[cpu] = 0;

    if (!plic_base)
        return;

    for (irq = 1; irq <= plic_ndev && irq < PLIC_MAX_SOURCES; irq++)
    {
        cpu = plic_irqcpu[irq];
        if (cpu && cpu < cpus)
            given[cpu]++;
    }
}

/* Whether a CPU has a context of its own to take interrupts in */
int krnPLICCPUUsable(unsigned int cpu)
{
    return (plic_base && plic_cpu_context(cpu) >= 0);
}

int krnPLICGetTarget(unsigned int irq)
{
    if (!plic_base || !irq || irq > plic_ndev || irq >= PLIC_MAX_SOURCES)
        return -1;

    return plic_irqcpu[irq];
}

/*
 * One claim from one context.
 *
 * On a sound controller that is a single read. The UltraRISC PLIC
 * loses pending interrupts when the claim register is read with more
 * than one enabled source pending, so there every other enabled source
 * is masked for the duration of the read and the lowest pending one is
 * the only candidate - the workaround Linux ships for the same part.
 * Nothing pending and enabled means nothing to claim, without a read.
 */
static unsigned int plic_claim_ctx(int ctx)
{
    uint32_t     saved[32];     /* enable words: the PLIC has at most 1023 sources */
    uint32_t     pend = 0, iso;
    unsigned int ngroups = (plic_ndev + 32) / 32, i, j, found = 0, src;
    unsigned long sstatus;

    if (!(plic_quirks & KRN_PLIC_QUIRK_CLAIM))
        return plic_read(PLIC_CLAIM(ctx));

    if (ngroups > 32)
        ngroups = 32;

    sstatus = plic_lock_acquire();

    /* Every enable word is read (all are restored); the first pending wins */
    for (i = 0; i < ngroups; i++)
    {
        saved[i] = plic_read(PLIC_ENABLE(ctx, i * 32));
        if (!pend && saved[i])
        {
            pend = saved[i] & plic_read(PLIC_PENDING(i * 32));
            if (pend)
                found = i;
        }
    }

    if (!pend)
    {
        plic_lock_release(sstatus);
        return 0;
    }

    iso = pend & -pend;

    for (j = 0; j < ngroups; j++)
    {
        uint32_t want = (j == found) ? iso : 0;

        if (saved[j] != want)
            plic_write(PLIC_ENABLE(ctx, j * 32), want);
    }

    src = plic_read(PLIC_CLAIM(ctx));

    for (j = 0; j < ngroups; j++)
    {
        uint32_t written = (j == found) ? iso : 0;

        if (saved[j] != written)
            plic_write(PLIC_ENABLE(ctx, j * 32), saved[j]);
    }

    plic_lock_release(sstatus);
    return src;
}

/*
 * Which source is asking the running hart. Returns 0 when nothing is
 * pending in its context, which is possible even inside the handler -
 * the source may have been masked or given to another CPU since it
 * raised the interrupt.
 *
 * Only the hart's own context is asked, the one the sources delivered
 * to this CPU are enabled in (krnPLICEnable() looks it up the same
 * way). Claiming from another would run a handler on a CPU its source
 * was not given to.
 *
 * *ctx is set to the context the source was claimed from and has to be
 * handed back to krnPLICComplete(). It lives with the caller, not here:
 * more than one hart can be between a claim and its completion, and a
 * completion only counts on the context that made the claim.
 */
unsigned int krnPLICClaim(int *ctx)
{
    int own;

    if (!plic_base)
        return 0;

    own = plic_cpu_context(krnRVCPUNumber());
    if (own < 0)
        return 0;

    *ctx = own;
    return plic_claim_ctx(own);
}

/*
 * What the controller thinks is going on.
 *
 * A source that a device has asserted shows up in the pending bits
 * whether or not it was ever delivered, so this says which of the two
 * halves is broken: nothing pending means the device never raised the
 * line, pending-but-undelivered means the fault is on our side.
 */
void krnPLICDumpState(unsigned int irq)
{
    unsigned int c;

    if (!plic_base)
        return;

    krnSBIPutStr("[plic] source ");
    krnSBIPutDec(irq);
    krnSBIPutStr(": priority ");
    krnSBIPutDec(plic_read(PLIC_PRIORITY(irq)));
    krnSBIPutStr(", pending ");
    krnSBIPutDec((plic_read(PLIC_PENDING(irq)) >> (irq & 31)) & 1);
    krnSBIPutStr(", enabled in");
    for (c = 0; c < plic_nctx; c++)
    {
        if ((plic_read(PLIC_ENABLE(plic_ctxlist[c], irq)) >> (irq & 31)) & 1)
        {
            krnSBIPutStr(" ");
            krnSBIPutDec((unsigned int)plic_ctxlist[c]);
        }
    }
    krnSBIPutStr("\n[plic] sie ");
    krnSBIPutHex(csr_read(sie));
    krnSBIPutStr(" sip ");
    krnSBIPutHex(csr_read(sip));
    krnSBIPutStr(" threshold ");
    krnSBIPutDec(plic_read(PLIC_THRESHOLD(plic_ctxlist[0])));
    if (irq && irq < PLIC_MAX_SOURCES)
    {
        krnSBIPutStr(" target cpu ");
        krnSBIPutDec(plic_irqcpu[irq]);
    }
    krnSBIPutStr("\n");
}

/*
 * Tell the controller the source has been serviced, on the context
 * krnPLICClaim() said it was claimed from.
 *
 * The controller ignores a completion for a source that is not enabled
 * on that context, and a source left uncompleted is never raised again.
 * It can have been masked there since the claim - its last handler
 * removed, or the source given to another CPU - so one found masked is
 * unmasked for just the write. Only the hart that owns the context
 * claims from it, and that is the caller, so nothing can be claimed
 * through the bit while it is set; the lock keeps the enable word from
 * changing underneath.
 */
void krnPLICComplete(int ctx, unsigned int irq)
{
    uint32_t      bits, bit = 1U << (irq & 31);
    unsigned long sstatus;

    if (!plic_base || !irq || ctx < 0)
        return;

    sstatus = plic_lock_acquire();

    bits = plic_read(PLIC_ENABLE(ctx, irq));
    if (!(bits & bit))
        plic_write(PLIC_ENABLE(ctx, irq), bits | bit);

    plic_write(PLIC_CLAIM(ctx), irq);

    if (!(bits & bit))
        plic_write(PLIC_ENABLE(ctx, irq), bits);

    plic_lock_release(sstatus);
}
