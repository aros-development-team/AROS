/*
 * Machine-specific definitions.
 *
 * This file needs to be replaced for every machine. Hosted ports
 * may share the same file in arch/all-$(ARCH)/kernel/kernel_arch.h
 *
 * This file is just a sample providing necessary minimum.
 */
#include "kernel_cpu.h"

/* Number of IRQs used in the machine. Needed by kernel_base.h */
#define IRQ_COUNT 96

/*
 * Interrupt controller functions.
 */

extern void ictl_enable_irq(uint8_t irq, struct KernelBase *KernelBase);
extern void ictl_disable_irq(uint8_t irq, struct KernelBase *KernelBase);
