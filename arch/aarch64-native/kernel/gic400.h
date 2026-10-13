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

#endif /* KERNEL_GIC400_H */
