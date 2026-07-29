/*
 * Machine-specific definitions for AArch64 native kernel.
 */
#include "kernel_cpu.h"

/* Number of IRQs used in the machine. Needed by kernel_base.h.
   A GIC presents shared peripheral interrupts well above the range of the
   earlier Broadcom controller: the SD host lands at 158 and PCIe legacy
   interrupts at 175, and a handler for anything beyond this count is
   silently refused. */
#define IRQ_COUNT 256

/*
 * Interrupt controller functions.
 */

extern void ictl_enable_irq(uint8_t irq, struct KernelBase *KernelBase);
extern void ictl_disable_irq(uint8_t irq, struct KernelBase *KernelBase);
