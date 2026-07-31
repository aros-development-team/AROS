/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Early kernel startup for the opensbi-riscv64 target.

    Entered from startup.S in S-mode with the boot hart id and the
    physical address of the device tree blob, as handed over by OpenSBI.

    Announces itself on the SBI console, parses the DTB for the memory
    layout, and prepares the KRN_* boot TagItem list. The remaining
    bring-up (MMU, interrupt dispatch, kernel.resource / exec handover)
    hangs off the end of kernel_cstart() as it is implemented.
*/

#include <inttypes.h>

#include <exec/execbase.h>
#include <utility/tagitem.h>
#include <aros/kernel.h>
#include <asm/cpu.h>

#include "kernel_sbi.h"
#include "kernel_intern.h"

/* The hart we were booted on (see kernel_intern.h) */
unsigned long __boot_hartid;

struct ExecBase *SysBase __attribute__((section(".data"))) = NULL;

/* Linker script symbols delimiting the kernel image */
extern char __text_start[];
extern char __kernel_end[];

#define BOOTTAG_MAX 10
static struct TagItem BootMsg[BOOTTAG_MAX];

struct TagItem *krnPrepareBootTags(void *fdt, struct krnFDTInfo *info)
{
    struct TagItem *tag = BootMsg;

    tag->ti_Tag  = KRN_KernelBase;
    tag->ti_Data = (IPTR)__text_start;
    tag++;
    tag->ti_Tag  = KRN_KernelLowest;
    tag->ti_Data = (IPTR)__text_start;
    tag++;
    tag->ti_Tag  = KRN_KernelHighest;
    tag->ti_Data = (IPTR)__kernel_end;
    tag++;
    tag->ti_Tag  = KRN_MEMLower;
    tag->ti_Data = (IPTR)__kernel_end;
    tag++;
    tag->ti_Tag  = KRN_MEMUpper;
    tag->ti_Data = (IPTR)(info->mem_base + info->mem_size);
    tag++;
    tag->ti_Tag  = KRN_FlattenedDeviceTree;
    tag->ti_Data = (IPTR)fdt;
    tag++;
    tag->ti_Tag  = KRN_BootLoader;
    tag->ti_Data = (IPTR)"OpenSBI";
    tag++;
    if (info->bootargs)
    {
        tag->ti_Tag  = KRN_CmdLine;
        tag->ti_Data = (IPTR)info->bootargs;
        tag++;
    }
    tag->ti_Tag  = TAG_DONE;
    tag->ti_Data = 0;

    return BootMsg;
}

void __attribute__((noreturn)) kernel_cstart(unsigned long hartid, void *fdt)
{
    struct krnFDTInfo fdtinfo;
    struct TagItem *msg;

    __boot_hartid = hartid;

    krnSBIPutStr("AROS64/riscv (OpenSBI)\n");
    krnSBIPutStr("boot hart: ");
    krnSBIPutHex(hartid);
    krnSBIPutStr("\ndtb:       ");
    krnSBIPutHex((uint64_t)(uintptr_t)fdt);
    krnSBIPutStr("\nkernel:    ");
    krnSBIPutHex((uint64_t)(uintptr_t)__text_start);
    krnSBIPutStr(" - ");
    krnSBIPutHex((uint64_t)(uintptr_t)__kernel_end);
    krnSBIPutStr("\n");

    if (krnParseFDT(fdt, &fdtinfo))
    {
        krnSBIPutStr("memory:    ");
        krnSBIPutHex(fdtinfo.mem_base);
        krnSBIPutStr(" - ");
        krnSBIPutHex(fdtinfo.mem_base + fdtinfo.mem_size);
        krnSBIPutStr(" (");
        krnSBIPutDec(fdtinfo.mem_size >> 20);
        krnSBIPutStr(" MiB)\ncpus:      ");
        krnSBIPutDec(fdtinfo.ncpus);
        krnSBIPutStr("\ntimebase:  ");
        krnSBIPutDec(fdtinfo.tb_freq);
        krnSBIPutStr(" Hz\n");
        if (fdtinfo.bootargs)
        {
            krnSBIPutStr("bootargs:  ");
            krnSBIPutStr(fdtinfo.bootargs);
            krnSBIPutStr("\n");
        }
    }
    else
    {
        krnSBIPutStr("[boot] WARNING: could not parse the device tree!\n");
        fdtinfo.mem_base = 0x80000000UL;
        fdtinfo.mem_size = 128 << 20;
    }

    msg = krnPrepareBootTags(fdt, &fdtinfo);
    (void)msg;
    krnSBIPutStr("boot tags prepared.\n");

    krnInitMMU(&fdtinfo);

    /* Start the 100Hz scheduler heartbeat and enable S-mode
       interrupt delivery */
    if (fdtinfo.tb_freq)
    {
        krnTimerInit(fdtinfo.tb_freq, 100);
        csr_set(sstatus, SSTATUS_SIE);
        krnSBIPutStr("timer:     100 Hz tick started.\n");
    }

    /* Command line debug switches: "testtrap" provokes a load access
       fault to exercise the trap handler, "testtimer" counts off three
       seconds of timer ticks */
    if (fdtinfo.bootargs)
    {
        const char *s;
        for (s = fdtinfo.bootargs; *s; s++)
        {
            if (s[0] == 't' && s[1] == 'e' && s[2] == 's' && s[3] == 't')
            {
                if (s[4] == 't' && s[5] == 'r' && s[6] == 'a' && s[7] == 'p')
                {
                    krnSBIPutStr("[boot] testtrap requested:\n");
                    (void)*(volatile unsigned long *)8;
                }
                if (s[4] == 't' && s[5] == 'i' && s[6] == 'm' && s[7] == 'e')
                {
                    uint64_t next = 100;
                    krnSBIPutStr("[boot] testtimer requested:\n");
                    while (next <= 300)
                    {
                        asm volatile("wfi");
                        if (__timer_ticks >= next)
                        {
                            krnSBIPutStr("  tick ");
                            krnSBIPutDec(__timer_ticks);
                            krnSBIPutStr(" (");
                            krnSBIPutDec(next / 100);
                            krnSBIPutStr("s)\n");
                            next += 100;
                        }
                    }
                }
            }
        }
    }

    /*
     * TODO: bring up the remaining harts via HSM, and hand the TagItem
     *       list over to the kernel.resource / exec bring-up
     *       (krnPrepareExecBase + InitCode).
     */
    krnSBIPutStr("early startup complete - halting.\n");

    for (;;)
        asm volatile("wfi");
}
