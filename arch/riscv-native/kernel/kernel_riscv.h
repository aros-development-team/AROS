#ifndef KERNEL_RISCV_H_
#define KERNEL_RISCV_H_
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Shared declarations for native RISC-V kernels.

    riscv-native holds the firmware-independent part of the kernel:
    per-hart TLS, the CPU numbering and masks, spinlocks, the SMP
    scheduler and the IPI core. Whatever needs the firmware - starting
    a hart, raising an inter-processor interrupt, remote TLB fences - is
    reached through the hook table below, which the platform (e.g.
    riscv64-sbi over SBI) fills in. It follows the ARM_Implementation
    / ARMI_* hooks of the ARM ports.
*/

#include <aros/config.h>
#include <exec/types.h>
#include <inttypes.h>

#include "tls.h"

struct KernelBase;
struct ExecBase;

/*
 * The most harts a native RISC-V kernel numbers. CPU masks are a fixed
 * RISCV_CPUMASK_WORDS ULONGs for the lifetime of the boot, so a mask
 * never has to be resized when a hart comes online. Platforms may
 * describe fewer (riscv64-sbi: KRN_MAX_HARTS).
 */
#define RISCV_MAXCPUS           32
#define RISCV_CPUMASK_WORDS     ((RISCV_MAXCPUS + 31) / 32)

struct RISCV_Implementation
{
    /*
     * Start the hart for logical CPU cpu at __secondary_entry with tp
     * = tls. Returns 0 once the firmware has accepted the request; the
     * hart reports in through cpu_Register().
     */
    int     (*RVI_StartCPU)(uint32_t cpu, tls_t *tls);
    /* Raise the software interrupt on every CPU in mask */
    void    (*RVI_SendIPI)(const ULONG *mask);
    /* Clear the calling hart's software interrupt (the IPI acknowledge) */
    void    (*RVI_AckIPI)(void);
    /* Per-hart platform setup, on that hart, before it schedules */
    void    (*RVI_InitCore)(struct KernelBase *, struct ExecBase *);
    /* Start the hart's own scheduler tick, on that hart */
    void    (*RVI_InitTimerCore)(void);
    /* sfence.vma for [va, va + size) on every CPU in mask (size 0: all) */
    void    (*RVI_RemoteSFence)(const ULONG *mask, IPTR va, IPTR size);
    /* Take the calling hart out of service for good (does not return) */
    void    (*RVI_StopCPU)(void);
    /* rdtime ticks per second */
    UQUAD   RVI_TimebaseFreq;
};

#if defined(__AROSEXEC_SMP__)
/* Start the secondary harts (kernel_cpusmp.c) - called by exec once
   every task has an affinity mask (exec_smp.c) */
void krnRVStartCPUs(struct KernelBase *KernelBase, struct ExecBase *SysBase);
/* Stop every other hart before the kernel is restarted in place; FALSE
   if one did not stop in time (kernel_riscv.c) */
BOOL krnRVStopOtherCPUs(void);
/* Forget every secondary hart - on (re)entry of the boot hart */
void krnRVResetCPUs(void);
#endif

extern struct RISCV_Implementation __riscv_arosintern;

/*
 * The time counter, RVI_TimebaseFreq ticks per second. rdtime fills one
 * register, so rv32 reads the two halves separately and retries if the
 * high half moved in between.
 */
static inline UQUAD krnRVReadTime(void)
{
#if __riscv_xlen == 32
    ULONG hi, lo, hi2;

    do
    {
        asm volatile("rdtimeh %0" : "=r"(hi));
        asm volatile("rdtime %0" : "=r"(lo));
        asm volatile("rdtimeh %0" : "=r"(hi2));
    } while (hi != hi2);
    return ((UQUAD)hi << 32) | lo;
#elif __riscv_xlen == 64
    UQUAD v;

    asm volatile("rdtime %0" : "=r"(v));
    return v;
#else
#error "kernel_riscv.h: __riscv_xlen must be 32 or 64"
#endif
}

/*
 * Logical CPU numbering. CPU 0 is the boot hart; the others are
 * numbered in the order they are brought online. Only the firmware
 * calls deal in hart ids.
 */
extern uint32_t __riscv_cpu_count;                      /* online CPUs     */
extern ULONG    __riscv_cpu_online[RISCV_CPUMASK_WORDS];/* online mask     */
#if defined(__AROSEXEC_SMP__)
extern tls_t    *__riscv_tls[RISCV_MAXCPUS];            /* by CPU number   */
extern tls_t    __riscv_boot_tls;                       /* CPU 0           */
#endif

/*
 * Resolve an affinity argument to a bitmap of online CPUs.
 * TASKAFFINITY_ANY and TASKAFFINITY_ALL_BUT_SELF are sentinels rather
 * than mask buffers; NULL is not a valid mask and selects nothing.
 */
static inline void krnRVResolveMask(const void *mask, uint32_t self,
                                    ULONG out[RISCV_CPUMASK_WORDS])
{
    int i;

    for (i = 0; i < RISCV_CPUMASK_WORDS; i++)
    {
        if ((IPTR)mask == (IPTR)-1)                 /* TASKAFFINITY_ANY         */
            out[i] = __riscv_cpu_online[i];
        else if ((IPTR)mask == (IPTR)-2)            /* TASKAFFINITY_ALL_BUT_SELF */
            out[i] = __riscv_cpu_online[i];
        else if (mask)
            out[i] = ((const ULONG *)mask)[i] & __riscv_cpu_online[i];
        else
            out[i] = 0;
    }
    if ((IPTR)mask == (IPTR)-2 && self < RISCV_MAXCPUS)
        out[self / 32] &= ~(1UL << (self % 32));
}

/*
 * Idle time per CPU, in rdtime ticks: what the idle tasks spent asleep
 * in krnRVIdleWait(). The platform's tick turns it into each CPU's load.
 */
extern volatile UQUAD __riscv_cpu_idletime[RISCV_MAXCPUS];

/* Sleep until the next interrupt, counting the time as idle (kernel_riscv.c) */
void krnRVIdleWait(void);

/* Mark a logical CPU online (kernel_riscv.c) */
void krnRVCPUOnline(uint32_t cpu);

/* The CPU number of the caller; 0 on uniprocessor builds */
static inline uint32_t krnRVCPUNumber(void)
{
#if defined(__AROSEXEC_SMP__)
    return TLS_GET(CPUNumber);
#else
    return 0;
#endif
}

/*
 * Trap nesting depth: per hart in TLS on SMP, a single global
 * otherwise (traps.S keeps both in step with this).
 */
#if defined(__AROSEXEC_SMP__)
#define KRN_TRAPDEPTH   (TLS_PTR_GET()->TrapDepth)
#else
extern int __riscv64_trap_depth;
#define KRN_TRAPDEPTH   __riscv64_trap_depth
#endif

/* Spin-wait hint (Zihintpause, part of RVA22; a nop elsewhere) */
static inline void krnRVPause(void)
{
    asm volatile(".insn i 0x0F, 0, x0, x0, 0x010" ::: "memory");
}

#endif /* KERNEL_RISCV_H_ */
