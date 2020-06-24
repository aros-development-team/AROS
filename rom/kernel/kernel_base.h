#ifndef KERNEL_BASE_H
#define KERNEL_BASE_H

/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#ifndef __KERNEL_NOLIBBASE__
#define __KERNEL_NOLIBBASE__
#endif /* __KERNEL_NOLIBBASE__ */

#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <aros/kernel.h>

/* Early declaration for ictl functions */
struct KernelBase;

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
};
#define KERNELIRQ_LIST(x)       KernelBase->kb_Interrupts[x].ki_List
#else
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
    ULONG               kb_ContextFlags;            /* Hints for KrnCreateContext()         */
    ULONG               kb_ContextSize;	                /* Total length of CPU context          */
    ULONG               kb_PageSize;                /* Physical memory page size            */
    APTR                kb_TimeSource;                  /* active/preferred timesource resource */
    struct PlatformData *kb_PlatformData;
#ifdef KERNELIRQ_NEEDSCONTROLLERS
    UBYTE               kb_ICTypeBase;                  /* used to set IC controller ID's       */
#endif
};

/*
 * Some useful global variables. They are global because:
 * - BootMsg needs to be stored before KernelBase is allocated;
 * - KernelBase is needed by interrupt handling code
 */
extern struct TagItem *BootMsg;
extern struct KernelBase *KernelBase;

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

#ifdef KERNELIRQ_NEEDSCONTROLLERS
#include <kernel_interruptcontrollers.h>
#endif

#endif /* !KERNEL_BASE_H */
