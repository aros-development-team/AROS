/*
 * Machine-specific definitions for AArch64 native kernel.
 */
#include "kernel_cpu.h"

#include <kernel_irqtypes.h>   /* irqid_t - this header may be reached without kernel_base.h */

/* Number of IRQs used in the machine. Needed by kernel_base.h.
   A GIC presents SPIs well above the earlier Broadcom controller's range
   (BCM2712: SD host at 305, RP1/PCIe higher still), and KrnAddIRQHandler
   silently refuses anything at or above the count - HW_IRQ_COUNT must be
   set explicitly, or kernel_base.h defaults it to 256 - INTB_KERNEL. */
#define IRQ_COUNT       384
#define HW_IRQ_COUNT    IRQ_COUNT

/*
 * Interrupt controller functions.
 */

extern void ictl_enable_irq(irqid_t irq, struct KernelBase *KernelBase);
extern void ictl_disable_irq(irqid_t irq, struct KernelBase *KernelBase);
