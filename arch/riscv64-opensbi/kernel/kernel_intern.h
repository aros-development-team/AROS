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
void krnSBIPutC(char c);
int  krnSBIGetC(void);
void krnSBIPutStr(const char *s);
int krnMMUMapPages(unsigned long va, unsigned long pa, unsigned long size,
                   unsigned long perms);
int krnMMUUnmapPages(unsigned long va, unsigned long size);

void krnSBIPutHex(uint64_t val);
void krnSBIPutHex32(uint32_t val);
void krnSBIPutHex8(unsigned char val);
void krnSBIPutDec(uint64_t val);

/* S-mode trap handling (traps.S / kernel_traps.c) */
extern void __trap_entry(void);
struct ExceptionContext;
void krnTrapHandler(struct ExceptionContext *ctx, unsigned long scause,
                    unsigned long stval);

/* Early flattened device tree parsing (kernel_fdt.c) */
#define KRN_MAX_MEMREGIONS  8

struct krnMemRegion
{
    uint64_t    base;
    uint64_t    size;
};

struct krnFDTInfo
{
    uint64_t    mem_base;   /* Largest /memory range (the main heap) */
    uint64_t    mem_size;
    /* Every region the device tree reports - a board can have several
       banks, and they need not be contiguous */
    struct krnMemRegion regions[KRN_MAX_MEMREGIONS];
    unsigned int nregions;
    const char *bootargs;   /* /chosen bootargs, or NULL            */
    uint32_t    ncpus;      /* Number of cpu@ nodes under /cpus     */
    uint32_t    totalsize;  /* Total size of the DTB                */
    uint32_t    tb_freq;    /* /cpus timebase-frequency (Hz)        */
    uint64_t    initrd_start;/* /chosen linux,initrd-start (or 0)    */
    uint64_t    initrd_end;
    /*
     * The platform interrupt controller. Every device interrupt on a
     * RISC-V board arrives through it, so a driver can do nothing but
     * poll until this is up.
     */
    uint64_t    plic_base;
    uint64_t    plic_size;
    uint32_t    plic_ndev;  /* riscv,ndev - highest source number    */
    /* Index of this hart's supervisor context in the PLIC's
       interrupts-extended list, or -1 if it was not described */
    int         plic_context;
    /*
     * Every supervisor context the controller offers, in hart order.
     * S-mode cannot read its own hart id, so which of these is "ours"
     * rests on what the firmware said the boot hart was - and that is
     * not always right. Sources are enabled on all of them so delivery
     * does not depend on getting it correct.
     */
#define KRN_MAX_PLIC_CONTEXTS   16
    int         plic_contexts[KRN_MAX_PLIC_CONTEXTS];
    unsigned int plic_ncontexts;
};
int krnParseFDT(void *dtb, struct krnFDTInfo *info);
void krnDumpFDT(void *dtb);
void krnDumpACPI(void);

/* Platform interrupt controller (kernel_plic.c) */
int krnPLICInit(struct krnFDTInfo *info);
unsigned int krnPLICClaim(void);
void krnPLICComplete(unsigned int irq);
void krnPLICEnable(unsigned int irq, int enable);
void krnPLICDumpState(unsigned int irq);

/* Hardware interrupt dispatch (intr.c) */
void krnHandleExternalIRQ(void);

/* Timer tick (kernel_timer.c) */
extern volatile uint64_t __timer_ticks;
void krnTimerInit(uint32_t timebase_hz, uint32_t tick_hz);
void krnTimerTick(void);

/* Sv39 MMU (kernel_mmu.c) */
void krnInitMMU(struct krnFDTInfo *info);
void krnMMUSetPerms(IPTR lo, IPTR hi, unsigned long perms);
void krnMMUMapRegion(IPTR base, IPTR size, unsigned long perms);

/* Boot module loading (kernel_elf.c) */
int krnLoadPackage(void *pkg, IPTR pkgsize, IPTR memlow, IPTR memhigh,
                   IPTR *lo, IPTR *hi, IPTR *memused);

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
