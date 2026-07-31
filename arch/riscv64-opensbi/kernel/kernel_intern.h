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

/* Early SBI debug console (kernel_console.c) */
void krnSBIPutStr(const char *s);
void krnSBIPutHex(uint64_t val);
void krnSBIPutDec(uint64_t val);

/* S-mode trap handling (traps.S / kernel_traps.c) */
extern void __trap_entry(void);
struct ExceptionContext;
void krnTrapHandler(struct ExceptionContext *ctx, unsigned long scause,
                    unsigned long stval);

/* Early flattened device tree parsing (kernel_fdt.c) */
struct krnFDTInfo
{
    uint64_t    mem_base;   /* First /memory range                  */
    uint64_t    mem_size;
    const char *bootargs;   /* /chosen bootargs, or NULL            */
    uint32_t    ncpus;      /* Number of cpu@ nodes under /cpus     */
    uint32_t    totalsize;  /* Total size of the DTB                */
    uint32_t    tb_freq;    /* /cpus timebase-frequency (Hz)        */
};
int krnParseFDT(void *dtb, struct krnFDTInfo *info);

/* Timer tick (kernel_timer.c) */
extern volatile uint64_t __timer_ticks;
void krnTimerInit(uint32_t timebase_hz, uint32_t tick_hz);
void krnTimerTick(void);

/* Sv39 MMU (kernel_mmu.c) */
void krnInitMMU(struct krnFDTInfo *info);

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
