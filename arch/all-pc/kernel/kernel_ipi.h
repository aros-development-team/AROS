/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#ifndef __KERNEL_IPI_H_
#define __KERNEL_IPI_H_

#include <aros/types/spinlock_s.h>
#include <utility/hooks.h>
#include "kernel_base.h"
#include "apic_ia32.h"

/*
    Private KERNEL IPI messages.

    Each message is the offset of its vector from APIC_EXCEPT_IPI_NOP:
    core_DoIPI() sends vector APIC_EXCEPT_IPI_NOP + message, and the
    handler is registered with exactly that offset as its data
    (apic_ia32.c). They used to be counted from X86_CPU_EXCEPT_COUNT,
    one below APIC_EXCEPT_IPI_NOP, so every message travelled on the
    next vector up and IPI_CAUSE on the APIC error vector.
*/
#define IPI_NOP         (APIC_EXCEPT_IPI_NOP - APIC_EXCEPT_IPI_NOP)
#define IPI_STOP        (APIC_EXCEPT_IPI_STOP - APIC_EXCEPT_IPI_NOP)
#define IPI_RESUME      (APIC_EXCEPT_IPI_RESUME - APIC_EXCEPT_IPI_NOP)
#define IPI_RESCHEDULE  (APIC_EXCEPT_IPI_RESCHEDULE - APIC_EXCEPT_IPI_NOP)
#define IPI_CALL_HOOK   (APIC_EXCEPT_IPI_CALL_HOOK - APIC_EXCEPT_IPI_NOP)
#define IPI_CAUSE       (APIC_EXCEPT_IPI_CAUSE - APIC_EXCEPT_IPI_NOP)

int core_IPIHandle(struct ExceptionContext *regs, void *data1, struct KernelBase *KernelBase);
void core_DoIPI(uint8_t ipi_number, void *cpu_mask, struct KernelBase *KernelBase);
int core_DoCallIPI(struct Hook *hook, void *cpu_mask, int async, int nargs, IPTR *args, APTR _KB);

/* Claim/commit and cancellation of queued calls, so exec can check the
   target's lifetime together with the enqueue (rom/exec/signal.c) and sweep
   stale calls at task teardown (rom/exec/remtask.c). On pc a CallIPIEntry is
   a struct IPIHook from the free list. */
struct CallIPIEntry;

struct CallIPIEntry *core_ClaimCallIPI(int cpu);
void core_CommitCallIPI(struct CallIPIEntry *cie, int cpu, struct Hook *hook, int nargs, IPTR *args);
void core_AbortCallIPI(struct CallIPIEntry *cie, int cpu);
void core_CancelCallIPIs(APTR hookEntry, IPTR matchArg);

#define KERNEL_IPI_CALL_CANCELABLE

#define IPI_CALL_HOOK_MAX_ARGS  5

/*
    IPI Call hook
*/
struct IPIHook
{
    struct Hook     ih_Hook;
    IPTR            ih_Args[IPI_CALL_HOOK_MAX_ARGS];
    uint32_t *      ih_CPUDone;
    uint32_t *      ih_CPURequested;
    uint32_t *      ih_CPUClaimed;
    int             ih_Async;
    spinlock_t      ih_SyncLock;
};

#endif /* __KERNEL_IPI_H_ */
