#ifndef KERNEL_BASE_H
#define KERNEL_BASE_H

/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#ifndef __KERNEL_NOLIBBASE__
#define __KERNEL_NOLIBBASE__
#endif /* __KERNEL_NOLIBBASE__ */

#ifndef EXEC_EXECBASE_H
#include <exec/execbase.h>
#endif
#ifndef EXEC_MEMORY_H
#include <exec/memory.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif
#ifndef AROS_KERNEL_H
#include <aros/kernel.h>
#endif

#if defined(__AROSEXEC_SMP__)
#include <aros/types/spinlock_s.h>
#endif

/* Early declaration for ictl functions */
struct KernelBase;

/* irqid_t/icid_t/icinstid_t - kernel_arch.h uses them in its prototypes */
#include <kernel_irqtypes.h>

/* These two specify IRQ_COUNT and EXCEPTIONS_COUNT */
#include <kernel_arch.h>
#include <kernel_cpu.h>

/* Declare global variable at absolute address */
#define IMPORT_GLOBAL(var, addr) \
    asm(".globl " # var "\n"     \
        ".set " # var ", " # addr )

/* Platform-specific stuff. Black box here. */
struct PlatformData;

#ifndef HW_IRQ_COUNT
#ifdef HW_IRQ_BASE
#define HW_IRQ_COUNT    (256 - INTB_KERNEL - HW_IRQ_BASE)
#else
#define HW_IRQ_COUNT    (256 - INTB_KERNEL)
#endif
#endif /* !HW_IRQ_COUNT */
#ifndef KBL_INTERNAL
#define KBL_INTERNAL    0
#endif /* !KBL_INTERNAL */

#ifdef KERNELIRQ_NEEDSPRIVATE
struct KernelInt
{
    IPTR        ki_Priv;                        /* arch specific per-irq data */
    struct List ki_List;
#ifdef KERNELIRQ_NEEDSCONTROLLERS
    /*
     * Which controller serves this source. These used to live in the
     * chain's own lh_Type/l_pad, which are UBYTE by the exec ABI and so
     * could not name more than 256 of either.
     */
    icid_t      ki_ICId;
    icinstid_t  ki_ICInst;
#endif
};
#define KERNELIRQ_LIST(x)       KernelBase->kb_Interrupts[x].ki_List
#define KERNELIRQ_ICID(x)       KernelBase->kb_Interrupts[x].ki_ICId
#define KERNELIRQ_ICINST(x)     KernelBase->kb_Interrupts[x].ki_ICInst
#else
#ifdef KERNELIRQ_NEEDSCONTROLLERS
#error interrupt controllers need the per-IRQ private area (KERNELIRQ_NEEDSPRIVATE)
#endif
#define KernelInt List
#define KERNELIRQ_LIST(x)       KernelBase->kb_Interrupts[x]
#endif

/* kernel.resource base. Nothing spectacular, really. */
struct KernelBase
{
    struct Node         kb_Node;
#ifdef KERNELIRQ_NEEDSCONTROLLERS
    struct List         kb_ICList;                      /* list of all controller types         */
    struct List         kb_InterruptMappings;
#endif
    struct MinList      kb_Exceptions[EXCEPTIONS_COUNT];
    struct KernelInt    kb_Interrupts[HW_IRQ_COUNT];
#if defined(__AROSEXEC_SMP__)
    spinlock_t          kb_IntrSpinLock;            /* guards kb_Exceptions/kb_Interrupts chains */
#endif
    ULONG               kb_ContextFlags;            /* Hints for KrnCreateContext()         */
    ULONG               kb_ContextSize;	                /* Total length of CPU context          */
    ULONG               kb_PageSize;                /* Physical memory page size            */
    APTR                kb_ClockSource;                  /* active/preferred clocksource resource */
    IPTR                kb_ClockUnit;
    struct PlatformData *kb_PlatformData;
#ifdef KERNELIRQ_NEEDSCONTROLLERS
    icid_t              kb_ICTypeBase;                  /* used to set IC controller ID's       */
#endif
    KrnSymResolver_t    kb_gResolver;
    APTR                kb_gResolvPrivate;
};

/*
 * Some useful global variables. They are global because:
 * - BootMsg needs to be stored before KernelBase is allocated;
 * - KernelBase is needed by interrupt handling code
 */
extern struct TagItem *BootMsg;
#ifndef __KERNEL_NOEXTERNBASE__
extern struct KernelBase *KernelBase;
#endif

/* Allocation function */
struct KernelBase *AllocKernelBase(struct ExecBase *SysBase);

/* Utility function to clear BSS segments. Call it before storing any globals!!! */
void __clear_bss(const struct KernelBSS *bss);

/* Memory header initialization functions */
void krnCreateMemHeader(CONST_STRPTR name, BYTE pri, APTR start, IPTR size, ULONG flags);
struct MemHeader *krnCreateROMHeader(CONST_STRPTR name, APTR start, APTR end);
/* Memhry header - TLSF support functions */
void krnCreateTLSFMemHeader(CONST_STRPTR name, BYTE pri, APTR start, IPTR size, ULONG flags);
struct MemHeader * krnConvertMemHeaderToTLSF(struct MemHeader * source);
void *krnAddExceptionHandler(UBYTE num, APTR handler,  APTR handlerData, APTR handlerData2, struct KernelBase *KernelBase);

#ifdef KERNELIRQ_NEEDSCONTROLLERS
#include <kernel_interruptcontrollers.h>
#endif

#endif /* !KERNEL_BASE_H */
