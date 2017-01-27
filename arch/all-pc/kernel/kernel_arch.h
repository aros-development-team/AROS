#ifndef _KERNEL_ARCH_H_
#define _KERNEL_ARCH_H_

/* Machine-specific definitions for IBM PC hardware */

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

/* Originally we didn't have disable function. Perhaps there was some reason. */
#define ictl_disable_irq(irq, base)

#endif /* !_KERNEL_ARCH_H_ */
