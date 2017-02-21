#ifndef _KERNEL_ARCH_H_
#define _KERNEL_ARCH_H_
/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Machine-specific definitions for IBM PC hardware
    Lang: english
*/

#define KERNELIRQ_NEEDSPRIVATE
#define KERNELIRQ_NEEDSCONTROLLERS
/*
 * Simulate SysBase access
 * Disabled because global SysBase is moved away from zeropage.
 *
#define EMULATE_SYSBASE*/

struct PlatformData;

/* Hardware IRQ's ********************************************************************************/

#include "apic_ia32.h"

/* by default we only know about the xtpic's irq's */
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

#if (__WORDSIZE==64)
static inline unsigned long long RDTSC() {
   unsigned long _tsc_upper, _tsc_lower;
   asm volatile (".byte 0x0f, 0x31" : "=a" (_tsc_lower), "=d"(_tsc_upper));
   return _tsc_lower | ((unsigned long long)_tsc_upper << 32);
} 
#else
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
