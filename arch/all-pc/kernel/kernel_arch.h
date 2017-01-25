#ifndef _KERNEL_ARCH_H_
#define _KERNEL_ARCH_H_

/* Machine-specific definitions for IBM PC hardware */

/* XT-PIC only has 16 IRQs */
#define IRQ_COUNT 16

/* We have several types of IRQ handlers and distinguish them by node type */
#define KBL_INTERNAL 0

/* Interrupt controller functions */
void ictl_enable_irq(unsigned char irq, struct KernelBase *KernelBase);

/* Originally we didn't have disable function. Perhaps there was some reason. */
#define ictl_disable_irq(irq, base)

#endif /* !_KERNEL_ARCH_H_ */
