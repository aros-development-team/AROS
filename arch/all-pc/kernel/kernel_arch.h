#ifndef _KERNEL_ARCH_H_
#define _KERNEL_ARCH_H_

struct PlatformData;

/* Machine-specific definitions for IBM PC hardware */

/* Hardware IRQ's ********************************************************************************/

/* XT-PIC only has 16 IRQs */
#define IRQ_COUNT       16

/*
 *  The first Hardware IRQ starts at 32
 *  (0 - 31 are cpu exceptions, see below..)
 */
#define HW_IRQ_BASE     32
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
void ictl_enable_irq(unsigned char irq, struct KernelBase *KernelBase);
void ictl_disable_irq(unsigned char irq, struct KernelBase *KernelBase);

/* CPU TImer *************************************************************************************/

#define ADDTIME(dest, src)			\
    (dest)->tv_micro += (src)->tv_micro;	\
    (dest)->tv_secs  += (src)->tv_secs;		\
    while((dest)->tv_micro > 999999)		\
    {						\
	(dest)->tv_secs++;			\
	(dest)->tv_micro -= 1000000;		\
    }

static inline unsigned long long RDTSC() {
   unsigned long long _tsc;
   asm volatile (".byte 0x0f, 0x31" : "=A" (_tsc));
   return _tsc;
} 

/* x86 specific SysCalls *************************************************************************/

struct syscallx86_Handler
{
        struct Node sc_Node;
        void (*sc_SysCall)();
};

#include "x86_syscalls.h"

BOOL krnAddSysCallHandler(struct PlatformData *, struct syscallx86_Handler *, BOOL, BOOL);

#endif /* !_KERNEL_ARCH_H_ */
