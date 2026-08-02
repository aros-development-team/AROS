/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Platform level interrupt controller for the opensbi-riscv64
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
    us is worked out from the device tree (see kernel_fdt.c).
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
static int           plic_context = -1;
static unsigned int  plic_ndev;

/*
 * Every supervisor context, and the one a claim last came from.
 *
 * Which context belongs to the running hart depends on a hart id that
 * S-mode cannot read for itself and that the firmware does not always
 * report correctly. Enabling on all of them, and claiming from
 * whichever actually has something pending, removes the guess.
 */
static int           plic_ctxlist[KRN_MAX_PLIC_CONTEXTS];
static unsigned int  plic_nctx;

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

    plic_nctx = info->plic_ncontexts;
    for (n = 0; n < plic_nctx; n++)
        plic_ctxlist[n] = info->plic_contexts[n];

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
 * Unmask a source and give it a usable priority, or mask it again.
 * Priority 1 is enough - the threshold is 0 and this kernel does not
 * rank device interrupts against each other.
 */
void krnPLICEnable(unsigned int irq, int enable)
{
    unsigned int c;

    if (!plic_base || !irq || irq > plic_ndev)
    {
        krnSBIPutStr("[plic] refusing source ");
        krnSBIPutDec(irq);
        krnSBIPutStr(" (ndev ");
        krnSBIPutDec(plic_ndev);
        krnSBIPutStr(")\n");
        return;
    }

    plic_write(PLIC_PRIORITY(irq), enable ? 1 : 0);

    for (c = 0; c < plic_nctx; c++)
    {
        uint32_t bits = plic_read(PLIC_ENABLE(plic_ctxlist[c], irq));

        if (enable)
            bits |= 1U << (irq & 31);
        else
            bits &= ~(1U << (irq & 31));
        plic_write(PLIC_ENABLE(plic_ctxlist[c], irq), bits);
    }
}

/*
 * Which source is asking. Returns 0 when nothing is pending, which is
 * possible even inside the handler - another hart may have taken it.
 */
unsigned int krnPLICClaim(void)
{
    unsigned int c;

    if (!plic_base)
        return 0;

    /*
     * Try the context believed to be this hart's first, then the rest.
     * Only the running hart can have anything pending, so whichever
     * answers is the right one - and it is remembered so the matching
     * completion goes back to the same place.
     */
    for (c = 0; c < plic_nctx; c++)
    {
        unsigned int src = plic_read(PLIC_CLAIM(plic_ctxlist[c]));

        if (src)
        {
            plic_context = plic_ctxlist[c];
            return src;
        }
    }

    return 0;
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
    krnSBIPutStr("\n");
}

/* Tell the controller the source has been serviced */
void krnPLICComplete(unsigned int irq)
{
    if (!plic_base || !irq || plic_context < 0)
        return;

    plic_write(PLIC_CLAIM(plic_context), irq);
}
