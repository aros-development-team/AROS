#ifndef ASM_TLS_H
#define ASM_TLS_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Per-hart data for native (supervisor-mode) RISC-V targets.

    The pointer lives in the tp register. Every task and every trap runs
    in S-mode, tp is not part of a task context (see traps.S), and the
    toolchains never use it for thread-local storage (gcc is configured
    --disable-tls and uses emutls; the Rust target has no native thread
    locals), so it keeps its value across task switches.

    Only SMP builds load tp. Uniprocessor builds keep the per-CPU state
    in ExecBase, exactly as before (see exec_platform.h).
*/

/* traps.S and startup.S reach these without a C compiler; kept in step
   with the structure by static assertions in kernel_riscv.c */
#define TLS_THISTASK_OFFSET     16
#define TLS_TRAPDEPTH_OFFSET    40
#define TLS_HARTID_OFFSET       48
#define TLS_STACKTOP_OFFSET     56

#ifndef __ASSEMBLER__

#include <exec/types.h>

typedef struct tls
{
    struct ExecBase     *SysBase;
    void *              *KernelBase;    /* Base of kernel.resource              */
    struct Task         *ThisTask;      /* Currently running task on this hart  */
    ULONG               ScheduleFlags;
    BYTE                IDNestCnt;
    BYTE                TDNestCnt;
    UWORD               Quantum;
    UWORD               Elapsed;
    UWORD               _pad0;
    ULONG               CPUNumber;      /* Logical CPU number, boot hart = 0    */
    /* RISC-V specific from here on */
    LONG                TrapDepth;      /* Trap nesting depth (traps.S, issuper)*/
    UQUAD               HartID;         /* SBI hart id, for firmware calls only */
    APTR                StackTop;       /* Initial sp for a secondary hart      */
    APTR                Platform;       /* Platform per-hart data (HartData)    */
} tls_t;

#define TLSSF_Quantum   (1 << 0)
#define TLSSF_Switch    (1 << 1)
#define TLSSF_Dispatch  (1 << 2)

#define TLS_OFFSET(name) ((char *)&(((tls_t *)0)->name)-(char *)0)

#define TLS_PTR_GET() \
    ({ \
        tls_t *__tls; \
        asm volatile("mv %0, tp":"=r"(__tls)); \
        __tls;  \
    })

#define TLS_GET(name) \
    ({ \
        tls_t *__tls; \
        asm volatile("mv %0, tp":"=r"(__tls)); \
        typeof(__tls -> name) __ret = (__tls -> name); \
        __ret;  \
    })

#define TLS_SET(name, val) \
    do { \
        tls_t *__tls; \
        asm volatile("mv %0, tp":"=r"(__tls)); \
        (__tls -> name) = val; \
    } while(0);

#endif /* !__ASSEMBLER__ */

#endif
