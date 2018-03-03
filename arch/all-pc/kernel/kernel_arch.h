#ifndef _KERNEL_ARCH_H_
#define _KERNEL_ARCH_H_
/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
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
    APTR                kb_APIC_TrampolineBase;	/* Starting address of secondary core bootstrap code	*/
    struct List         kb_SysCallHandlers;
    struct ACPIData     *kb_ACPI;
    struct APICData     *kb_APIC;
    struct IOAPICData   *kb_IOAPIC;
    struct List         kb_FreeIPIHooks;
    struct List         kb_BusyIPIHooks;
    spinlock_t          kb_FreeIPIHooksLock;
    spinlock_t          kb_BusyIPIHooksLock;
};

#define PLATFORMF_HAVEMSI       (1 << 1)

/* Hardware IRQ's ********************************************************************************/

/* By default we only know about the xtpic's IRQs */
#define IRQ_COUNT       I8259A_IRQCOUNT

/*
 *  The first Hardware IRQ starts at 32
 *  (0 - 31 are cpu exceptions, see below..)
 */
#define HW_IRQ_BASE     X86_CPU_EXCEPT_COUNT

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

/* CPU TImer *************************************************************************************/

#define ADDTIME(dest, src)			\
    (dest)->tv_micro += (src)->tv_micro;	\
    (dest)->tv_secs  += (src)->tv_secs;		\
    while((dest)->tv_micro > 999999)		\
    {						\
	(dest)->tv_secs++;			\
	(dest)->tv_micro -= 1000000;		\
    }

/* use the correct registers depending on arch. */
#if (__WORDSIZE==64)
#define INTR_USERMODESTACK        (regs->ss != 0)
static inline unsigned long long RDTSC() {
   unsigned long _tsc_upper, _tsc_lower;
   asm volatile (".byte 0x0f, 0x31" : "=a" (_tsc_lower), "=d"(_tsc_upper));
   return _tsc_lower | ((unsigned long long)_tsc_upper << 32);
} 
#else
#define INTR_USERMODESTACK        (regs->ds != KERNEL_DS)
static inline unsigned long long RDTSC() {
   unsigned long long _tsc;
   asm volatile (".byte 0x0f, 0x31" : "=A" (_tsc));
   return _tsc;
} 
#endif

/* x86 specific SysCalls *************************************************************************/

struct syscallx86_Handler
{
        struct Node sc_Node;
        void (*sc_SysCall)();
};

#include "x86_syscalls.h"

BOOL krnAddSysCallHandler(struct PlatformData *, struct syscallx86_Handler *, BOOL, BOOL);

#endif /* !_KERNEL_ARCH_H_ */
