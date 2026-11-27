/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: GIC-400 (GICv2) SMP support for the Pi 4 and Pi 5.
*/

#ifndef KERNEL_GIC400_H
#define KERNEL_GIC400_H

#include <exec/types.h>

#include <inttypes.h>

#define GIC400_IPI_SGI  0

void gic400_setbase(uintptr_t dist, uintptr_t cpuif);
void gic400_init_core(void);
void gic400_send_ipi(uint32_t, uint32_t, uint32_t);
void gic400_handle_ipi(void);

/*
 * Which core a shared interrupt (SPI) is delivered to. The caller of
 * gic400_irq_route() and gic400_irq_settarget() holds kb_IntrSpinLock in
 * write mode.
 */
void gic400_irq_route(int irq);
int gic400_irq_settarget(int irq, int cpu);
int gic400_irq_gettarget(int irq);

/*
 * What the dispatch loop calls for an interrupt it has acknowledged, in
 * place of krnRunIRQHandlers(): the handlers run only on the core the
 * interrupt is delivered to.
 */
struct KernelBase;
void gic400_run_irq(struct KernelBase *KernelBase, int irq);

#endif /* KERNEL_GIC400_H */
