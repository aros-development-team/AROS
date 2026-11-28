/*
 * Machine-specific definitions for sam4x0 native kernel.
 *
 * Derived from the template in rom/kernel/kernel_arch.h.
 *
 * Copyright (C) 2012-2026, The AROS Development Team. All rights reserved.
 *
 */
#include <asm/amcc440.h>
#include "kernel_cpu.h"

#include <kernel_irqtypes.h>   /* irqid_t - this header may be reached without kernel_base.h */

struct KernelBase;

/* Number of IRQs used in the machine. Needed by kernel_base.h */
#define IRQ_COUNT    256
#define HW_IRQ_COUNT IRQ_COUNT

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
