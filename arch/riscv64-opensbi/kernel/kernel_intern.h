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

/* Harts described by the device tree (kernel_startup.c) */
extern unsigned long __ncpus;

/* Index of the boot hart among the cpu@ nodes (kernel_startup.c) */
extern int __boot_cpu_index;

/* The S-mode time CSR - counts at the /cpus timebase-frequency */
static inline uint64_t krnReadTime(void)
{
    uint64_t v;
    asm volatile("rdtime %0" : "=r"(v));
    return v;
}

/* CPU load accounting (kernel_timer.c): time the boot hart spent in
   the dispatcher's wfi, and the load computed from it once a second */
extern volatile uint64_t __cpu_sleeptime;
extern volatile uint32_t __cpu_load;

/* /cpus timebase-frequency, for time <-> seconds (kernel_timer.c) */
extern uint64_t __timebase_freq;

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

/*
 * Per-hart data, modeled on the x86 APICData/CPUData pair: one entry
 * per hart the device tree describes, filled at boot so the secondary
 * harts can be brought up from it in due course. Reached through
 * KernelBase->kb_PlatformData.
 */
struct HartData
{
    uint64_t    hd_HartID;          /* SBI hart id (the cpu@ reg value)     */
    int         hd_PLICContext;     /* Supervisor context in the PLIC, or
                                       -1 when the tree describes none      */
    uint32_t    hd_Flags;
    /* Load accounting (kernel_timer.c) */
    uint64_t    hd_SleepTime;
    uint64_t    hd_LastLoadTime;
    uint32_t    hd_Load;
};

#define HARTF_BOOT      (1 << 0)    /* The hart AROS booted on              */
#define HARTF_ONLINE    (1 << 1)    /* Executing AROS                       */

struct PlatformData
{
    uint32_t        kb_HartCount;
    int             kb_BootHart;    /* Index into kb_Harts                  */
    uint32_t        kb_TimebaseFreq;/* /cpus timebase-frequency, Hz         */
    struct HartData kb_Harts[0];
};

/* Early flattened device tree parsing (kernel_fdt.c) */
#define KRN_MAX_MEMREGIONS  8
#define KRN_MAX_HARTS       16

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
    int         boot_cpu;   /* Index of the boot hart among them    */
    /* The hart id of each cpu@ node, in tree order */
    uint64_t    hartids[KRN_MAX_HARTS];
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

/* What the boot parse found, kept for the platform init to read
   (kernel_startup.c) */
extern struct krnFDTInfo *__bootfdtinfo;
void krnDumpACPI(void);
/* Describe the interrupt controller from the MADT when there is no
   device tree; returns 0 when ACPI says nothing about it */
int  krnACPIPLICInfo(struct krnFDTInfo *info);

/* Platform interrupt controller (kernel_plic.c) */
int krnPLICInit(struct krnFDTInfo *info);
unsigned int krnPLICClaim(void);
void krnPLICComplete(unsigned int irq);
void krnPLICEnable(unsigned int irq, int enable);
void krnPLICDumpState(unsigned int irq);

/* Hardware interrupt dispatch (intr.c) */
void krnHandleExternalIRQ(void);

/* Per-source delivery counts (intr.c), sized for the largest PLIC
   riscv,ndev seen plus the unused source 0 */
#define KRN_MAX_IRQ_SOURCES 161
extern ULONG __irq_counts[KRN_MAX_IRQ_SOURCES];

/* How many sources the interrupt controller drives (kernel_plic.c) */
unsigned int krnPLICSourceCount(void);

/*
 * A controller collecting interrupts onto one of the platform's
 * sources - the message controller of a PCIe bridge (modifyirq.c).
 * Servicing that source reads its pending register and runs the
 * handlers of the sources its vectors were given.
 */
#define KRN_MAX_MSI_CONTROLLERS 4
struct krnMSIController
{
    unsigned int    irq;        /* the source it raises          */
    IPTR            status;     /* its pending register          */
    unsigned int    base;       /* source given to vector 0      */
    unsigned int    count;      /* how many vectors it has       */
};
extern struct krnMSIController __msi_ctrl[KRN_MAX_MSI_CONTROLLERS];
struct krnMSIController *krnFindMSIController(unsigned int irq);

/*
 * Sources above the controller's own, for interrupts with no wire -
 * see allocirq.c. One byte per number between the last real source
 * and HW_IRQ_COUNT.
 */
extern UBYTE __virq_used[];
ULONG krnAllocVirtualIRQ(ULONG count);
void krnFreeVirtualIRQ(ULONG start, ULONG count);

/* Timer tick (kernel_timer.c). The context is the interrupted state,
   for the serial debug poke; NULL when called from the idle loop. */
extern volatile uint64_t __timer_ticks;
void krnTimerInit(uint32_t timebase_hz, uint32_t tick_hz);
void krnTimerTick(struct ExceptionContext *ctx);

/* Sv39 MMU (kernel_mmu.c) */
void krnInitMMU(struct krnFDTInfo *info);
void krnMMUSetPerms(IPTR lo, IPTR hi, unsigned long perms);
void krnMMUMapRegion(IPTR base, IPTR size, unsigned long perms);

/* Boot module loading (kernel_elf.c) */
int krnLoadPackage(void *pkg, IPTR pkgsize, IPTR memlow, IPTR memhigh,
                   IPTR *lo, IPTR *hi, IPTR *memused);

/* Loaded-module descriptor chain for the KRN_DebugInfo boot tag
   (kernel_elf.c) */
extern void *__ks_debuginfo;

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
