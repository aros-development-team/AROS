/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Machine-specific kernel definitions for the opensbi-riscv64
          target.

    Replaces rom/kernel/kernel_arch.h, whose defaults turn the
    interrupt controller calls into no-ops.
*/

#ifndef KERNEL_ARCH_H
#define KERNEL_ARCH_H

#include "kernel_cpu.h"

/*
 * How many interrupt sources kernel.resource keeps handler chains for.
 * The PLIC specification allows up to 1023, but boards use far fewer
 * and each one costs a list in KernelBase; 256 covers what the generic
 * code can address anyway, since it passes the source as a uint8_t.
 */
#define IRQ_COUNT       256
#define HW_IRQ_COUNT    IRQ_COUNT

/*
 * Marker for sources that must not be built against the defaults.
 * kernel_base.h supplies its own HW_IRQ_COUNT when this header is
 * missing, and rom/kernel/kernel_arch.h turns the controller calls
 * into empty macros, so a translation unit that fails to find this one
 * still compiles - it just quietly gets a different interrupt count
 * and no unmasking. Anything that would be wrong that way asks for
 * this instead of trusting the include path.
 */
#define KERNEL_ARCH_IS_OPENSBI_RISCV64

/* Interrupt controller, implemented over the PLIC in intr.c */
extern void ictl_enable_irq(uint8_t irq, struct KernelBase *KernelBase);
extern void ictl_disable_irq(uint8_t irq, struct KernelBase *KernelBase);

#endif /* KERNEL_ARCH_H */
