/*
 * Machine-specific definitions.
 *
 * This file needs to be replaced for every machine. Hosted ports
 * may share the same file in arch/all-$(ARCH)/kernel/kernel_arch.h
 *
 * This file is just a sample providing necessary minimum.
 */
#include <asm/amcc440.h>
#include "kernel_cpu.h"

#include <kernel_irqtypes.h>   /* irqid_t - this header may be reached without kernel_base.h */

struct KernelBase;

/* Number of IRQs used in the machine. Needed by kernel_base.h */
#define IRQ_COUNT 256

/*
 * Interrupt controller functions. Actually have the following prototypes:
 *
 * void ictl_enable_irq(irqid_t num);
 * void ictl_disable_irq(irqid_t num);
 */

static inline void ictl_enable_irq(irqid_t irq, struct KernelBase *base)
{
    /* Call the architecture specific syscall */
    krnSysCall1(SC_IRQ_ENABLE, irq);
}

static inline void ictl_disable_irq(irqid_t irq, struct KernelBase *base)
{
    /* Call the architecture specific syscall */
    krnSysCall1(SC_IRQ_DISABLE, irq);
}
