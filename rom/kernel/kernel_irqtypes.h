#ifndef KERNEL_IRQTYPES_H
#define KERNEL_IRQTYPES_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Widths of the interrupt identity spaces kernel.resource passes around.
*/

#include <exec/types.h>
#include <aros/cpu.h>

/*
 * An interrupt source is identified by three separate numbers, and they do
 * not share a width:
 *
 *   irqid_t     the source number itself - the value KrnAddIRQHandler() is
 *               given, and the index into KernelBase->kb_Interrupts[]. On
 *               x86_64 this is the GSI space, on riscv the PLIC source.
 *   icid_t      which interrupt controller *type* serves that source.
 *   icinstid_t  which instance of that controller type.
 *
 * icintrid_t is the (icid, instance) pair packed into one value, as returned
 * by krnAddInterruptController(). Use ICINTR_MAKE()/ICINTR_ICID()/ICINTR_INST()
 * rather than open-coding the split - the shift is not a fixed 8 bits.
 *
 * An arch that needs more room than the defaults below says so from its
 * <aros/cpu.h>, which is where HW_IRQ_BASE already lives and which is read
 * before any of the kernel headers. Everything here is derived from those
 * three numbers, so an arch never has to name a type.
 *
 * Anything left at the defaults keeps the widths these APIs have always had,
 * which is what keeps m68k (and every other narrow port) unchanged.
 */

#ifndef KRN_IRQID_BITS
#define KRN_IRQID_BITS      16
#endif
#ifndef KRN_ICID_BITS
#define KRN_ICID_BITS       8
#endif
#ifndef KRN_ICINST_BITS
#define KRN_ICINST_BITS     8
#endif

#if (KRN_IRQID_BITS > 32) || (KRN_ICID_BITS > 16) || (KRN_ICINST_BITS > 16)
#error interrupt identity widths exceed what kernel.resource can carry
#endif
/*
 * KrnAddIRQHandler() takes a uint32_t, so a wider source number could not be
 * asked for through the public API even if the tables could hold it.
 */
#if ((KRN_ICID_BITS + KRN_ICINST_BITS) > 32)
#error packed icintrid_t would not fit in 32 bits
#endif

#if (KRN_IRQID_BITS <= 8)
typedef UBYTE   irqid_t;
#elif (KRN_IRQID_BITS <= 16)
typedef UWORD   irqid_t;
#else
typedef ULONG   irqid_t;
#endif

#if (KRN_ICID_BITS <= 8)
typedef UBYTE   icid_t;
#else
typedef UWORD   icid_t;
#endif

#if (KRN_ICINST_BITS <= 8)
typedef UBYTE   icinstid_t;
#else
typedef UWORD   icinstid_t;
#endif

#if ((KRN_ICID_BITS + KRN_ICINST_BITS) <= 8)
typedef UBYTE   icintrid_t;
#elif ((KRN_ICID_BITS + KRN_ICINST_BITS) <= 16)
typedef UWORD   icintrid_t;
#else
typedef ULONG   icintrid_t;
#endif

#define KRN_IRQID_MASK      ((ULONG)((1UL << KRN_IRQID_BITS) - 1))
#define KRN_ICID_MASK       ((ULONG)((1UL << KRN_ICID_BITS) - 1))
#define KRN_ICINST_MASK     ((ULONG)((1UL << KRN_ICINST_BITS) - 1))

/*
 * Failure markers. Spelled out rather than written as (type)-1 at each use,
 * so that widening a space does not silently move the value.
 *
 * There is deliberately no marker for irqid_t. A target whose HW_IRQ_COUNT
 * fills the whole width - x86_64, with 65536 sources in a 16-bit space -
 * has no value left over to spare, so a "no IRQ" sentinel has to come from
 * the caller's own wider type rather than from this one.
 */
#define KRN_ICID_INVALID    ((icid_t)KRN_ICID_MASK)
#define KRN_ICINTR_INVALID  ((icintrid_t)-1)

#define ICINTR_ICID(icintr)     ((icid_t)(((ULONG)(icintr) >> KRN_ICINST_BITS) & KRN_ICID_MASK))
#define ICINTR_INST(icintr)     ((icinstid_t)((ULONG)(icintr) & KRN_ICINST_MASK))
#define ICINTR_MAKE(icid, inst) ((icintrid_t)((((ULONG)(icid) & KRN_ICID_MASK) << KRN_ICINST_BITS) | \
                                              ((ULONG)(inst) & KRN_ICINST_MASK)))

#endif /* !KERNEL_IRQTYPES_H */
