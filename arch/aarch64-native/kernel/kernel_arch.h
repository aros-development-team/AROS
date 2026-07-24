/*
 * Machine-specific definitions for AArch64 native kernel.
 */
#include "kernel_cpu.h"

/* Number of IRQs used in the machine. Needed by kernel_base.h */
#define IRQ_COUNT 72

/*
 * Interrupt controller functions.
 */

extern void ictl_enable_irq(uint8_t irq, struct KernelBase *KernelBase);
extern void ictl_disable_irq(uint8_t irq, struct KernelBase *KernelBase);
