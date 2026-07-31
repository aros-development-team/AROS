/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: opensbi-riscv64 kernel internals.
*/

#ifndef KERNEL_INTERN_H_
#define KERNEL_INTERN_H_

#include <aros/libcall.h>
#include <inttypes.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <stdio.h>
#include <stdarg.h>

#undef KernelBase
struct KernelBase;

#define __STR(x) #x
#define STR(x) __STR(x)

/* The hart we were booted on, as handed over by OpenSBI */
extern unsigned long __boot_hartid;

/*
 * mhartid is an M-mode CSR and can not be read from S-mode; the boot
 * hart id comes from the SBI handoff. Once SMP bring-up exists the
 * per-hart id will live in the per-cpu data block (tp/sscratch) instead.
 */
static inline int GetCPUNumber(void)
{
    return (int)__boot_hartid;
}

#endif /* KERNEL_INTERN_H_ */
