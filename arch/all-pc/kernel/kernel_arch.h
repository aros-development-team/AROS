#ifndef _KERNEL_ARCH_H_
#define _KERNEL_ARCH_H_
/*
    Copyright © 1995-2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Machine-specific definitions for IBM PC hardware
    Lang: english
*/

#include <exec/nodes.h>
#include <exec/lists.h>
#include <aros/types/spinlock_s.h>

#include "apic_ia32.h"

#define PAGE_SIZE	        0x1000
#define PAGE_MASK	        0x0FFF

#define KERNELIRQ_NEEDSPRIVATE
#define KERNELIRQ_NEEDSCONTROLLERS
/*
 * Simulate SysBase access
 * Disabled because global SysBase is moved away from zeropage.
 */
/*#define EMULATE_SYSBASE*/

/* Platform-specific part of KernelBase */
struct PlatformData
{
    IPTR                kb_PDFlags;
    APTR                kb_APIC_TrampolineBase; /* Starting address of secondary core bootstrap code	*/
    APTR                kb_APICHeartBeat;
    struct List         kb_SysCallHandlers;
    struct ACPIData     *kb_ACPI;
    struct APICData     *kb_APIC;
    struct IOAPICData   *kb_IOAPIC;
    struct Interrupt    kb_APICResetHandler;
    struct List         kb_FreeIPIHooks;
    struct List         kb_BusyIPIHooks;
    spinlock_t          kb_FreeIPIHooksLock;
    spinlock_t          kb_BusyIPIHooksLock;
    ULONG               kb_LastException;
    ULONG               kb_LastExceptionError;
    union {
        ULONG           kb_LastState;
        struct {
            UWORD       kb_StateCtx;
            UWORD       kb_StateInt;
        };
    };
    struct IntrNode     *kb_LastIntr;   
    UWORD               kb_LastInt;
};

#define PLATFORMB_PRIMED        0
#define PLATFORMF_PRIMED        (1 << PLATFORMB_PRIMED)
#define PLATFORMB_HAVEHEARTBEAT 1
#define PLATFORMF_HAVEHEARTBEAT (1 << PLATFORMB_HAVEHEARTBEAT)
#define PLATFORMB_HAVEMSI       2
#define PLATFORMF_HAVEMSI       (1 << PLATFORMB_HAVEMSI)
/*
 * State flags used to express the current running "context"
 * used in both kb_PDFlags & kb_StateCtx
 */
#define PLATFORMB_INEXCPT       14
#define PLATFORMF_INEXCPT       (1 << PLATFORMB_INEXCPT)
#define PLATFORMB_INIRQ         15
#define PLATFORMF_INIRQ         (1 << PLATFORMB_INIRQ)

/* Hardware IRQs *********************************************************************************/

/* By default we only know about the xtpic's IRQs */
#define IRQ_COUNT       I8259A_IRQCOUNT

/*
 * We handle all 255 exception vectors. However vectors starting from 0x20
 * are hardware IRQs which are handled separately. So - 32 raw exceptions.
 */
#define EXCEPTIONS_COUNT (X86_CPU_EXCEPT_COUNT + APIC_CPU_EXCEPT_COUNT)

/*
    0 - Divide Error
    1 - Debug
    2 - NMI
    3 - Breakpoint
    4 - Overflow
    5 - Bound Range Exceeded
    6 - Invalid Opcode
    7 - Device Not Available
    8 - Double Fault
    9 - Coprocessor Segment Overrun
    10 - Invalid TSS
    11 - Segment Not Present
    12 - Stack Fault
    13 - GPF
    14 - Page Fault
    16 - x87 FPU Floating Point Error
    17 - Alignment Check
    18 - Machine-Check
    19 - SIMD Floating-Point
*/

/* Interrupt controller functions */
void ictl_enable_irq(unsigned char, struct KernelBase *);
void ictl_disable_irq(unsigned char, struct KernelBase *);
BOOL ictl_is_irq_enabled(unsigned char, struct KernelBase *);

#define IRQINTB_ENABLED 1
#define IRQINTF_ENABLED (1 << IRQINTB_ENABLED)

/* CPU Timer *************************************************************************************/

#define ADDTIME(dest, src)			\
    (dest)->tv_micro += (src)->tv_micro;	\
    (dest)->tv_secs  += (src)->tv_secs;		\
    while((dest)->tv_micro > 999999)		\
    {						\
	(dest)->tv_secs++;			\
	(dest)->tv_micro -= 1000000;		\
    }

/*
 * INTR_FROMUSERMODE uses arch-specific mechanisms to determine which CPL we
 * are returning to.
 * For x86-64, every task has the SS register initialized to a valid segment
 * descriptor. The descriptor itself isn't used by x86-64; however when a
 * privilege level switch occurs upon an interrupt, SS is reset to zero and
 * the old value is pushed to the stack as part of the interrupt context.
 * Both steps are done as part of the CPU's atomic interrupt mechanism, so
 * we can rely on the sequence not being only partly completed.
 * On x86-32, we rely on a similar mechanism, but using the CS register
 * instead of SS. CS is set to one value for user mode, but replaced with a
 * new value specific to the kernel privilege level when an interrupt occurs.
 */
#if (__WORDSIZE==64)
#define INTR_FROMUSERMODE (regs->ss != 0)
#else
#define INTR_FROMUSERMODE (regs->cs != KERNEL_CS)
#endif

static inline UQUAD RDTSC(void)
{
    unsigned lo, hi;
    /* Read the TSC without serializing */
    asm volatile(
        "rdtsc\n\t"
        : "=a"(lo), "=d"(hi)
        :
        :
    );
    return ((UQUAD)hi << 32) | lo;
}

static inline UQUAD rdtsc_start(void)
{
    unsigned lo, hi;
    /* Serialize previous instructions, then read TSC */
    asm volatile(
        "cpuid\n\t"
        "rdtsc\n\t"
        : "=a"(lo), "=d"(hi)
        : "a"(0)
        : "rbx", "rcx", "memory"
    );
    return ((UQUAD)hi << 32) | lo;
}

static inline UQUAD rdtsc_end(void)
{
    unsigned lo, hi;
#if (0)
    asm volatile("rdtscp\n\t" : "=a"(lo), "=d"(hi) : : "rcx");
#else
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi) : : "memory");
#endif
    /* serialize to ensure the rdtsc is ordered before later instructions */
    asm volatile("cpuid\n\t" : : : "rax", "rbx", "rcx", "rdx", "memory");
    return ((UQUAD)hi << 32) | lo;
}

/* x86 specific SysCalls *************************************************************************/

struct syscallx86_Handler
{
        struct Node sc_Node;
        void (*sc_SysCall)(APTR);
};

#include "x86_syscalls.h"

BOOL krnAddSysCallHandler(struct PlatformData *, struct syscallx86_Handler *, BOOL, BOOL);

#endif /* !_KERNEL_ARCH_H_ */
