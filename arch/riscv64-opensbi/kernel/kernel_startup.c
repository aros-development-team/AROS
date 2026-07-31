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
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/tasks.h>
#include <utility/tagitem.h>
#include <aros/kernel.h>
#include <asm/cpu.h>
#include <proto/exec.h>

#include "kernel_sbi.h"
#include "kernel_intern.h"
#include "kernel_romtags.h"

/* rom/kernel/kernel_memory.c provides no header for this */
void krnCreateMemHeader(CONST_STRPTR name, BYTE pri, APTR start, IPTR size,
                        ULONG flags);

/* The hart we were booted on (see kernel_intern.h) */
unsigned long __boot_hartid;

struct ExecBase *SysBase __attribute__((section(".data"))) = NULL;

/* Linker script symbols delimiting the kernel image */
extern char __text_start[];
extern char __kernel_end[];

#define BOOTTAG_MAX 10
static struct TagItem BootMsg[BOOTTAG_MAX];

/* "testsched" support: a second task signalling the boot task */
static struct Task *bootTask;
static ULONG schedSig;

static void schedTestEntry(void)
{
    krnSBIPutStr("[testsched] second task running!\n");
    Signal(bootTask, schedSig);
    Wait(0);        /* sleep forever */
}

static void krnSchedTest(void)
{
    struct Task *t;
    BYTE sigbit;
    struct TagItem tags[] =
    {
        { TASKTAG_NAME, (IPTR)"SchedTest"          },
        { TASKTAG_PRI,  5                          },
        { TASKTAG_PC,   (IPTR)schedTestEntry       },
        { TAG_DONE,     0                          }
    };

    bootTask = FindTask(NULL);
    sigbit = AllocSignal(-1);
    schedSig = 1UL << sigbit;

    krnSBIPutStr("[testsched] creating second task...\n");
    t = NewCreateTaskA(tags);
    if (!t)
    {
        krnSBIPutStr("[testsched] FAILED to create the task!\n");
        return;
    }

    krnSBIPutStr("[testsched] waiting for its signal...\n");
    Wait(schedSig);
    krnSBIPutStr("[testsched] signal received - context switching works!\n");
    FreeSignal(sigbit);
}

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
     * Hand over to exec: build the system memory header in the free RAM
     * after the kernel image, scan the kickstart for residents, create
     * ExecBase and run the resident init chain.
     *
     * TODO: bring up the remaining harts via HSM.
     */
    {
        struct MemHeader *mh;
        UWORD *ranges[3];
        IPTR memlow  = ((IPTR)__kernel_end + 4095) & ~(IPTR)4095;
        IPTR memhigh = fdtinfo.mem_base + fdtinfo.mem_size;
        IPTR dtbaddr = (IPTR)fdt;

        /* Keep the DTB (placed near the top of RAM by qemu/OpenSBI)
           out of the allocatable pool */
        if (dtbaddr >= memlow && dtbaddr < memhigh)
            memhigh = dtbaddr & ~(IPTR)4095;

        mh = (struct MemHeader *)memlow;
        krnCreateMemHeader("System Memory", 0, (APTR)memlow,
                           memhigh - memlow,
                           MEMF_FAST | MEMF_PUBLIC | MEMF_KICK | MEMF_LOCAL);

        ranges[0] = (UWORD *)__text_start;
        ranges[1] = (UWORD *)__kernel_end;
        ranges[2] = (UWORD *)-1;

        krnSBIPutStr("exec:      preparing ExecBase (memory ");
        krnSBIPutHex(memlow);
        krnSBIPutStr(" - ");
        krnSBIPutHex(memhigh);
        krnSBIPutStr(")\n");

        if (krnPrepareExecBase(ranges, mh, msg))
        {
            krnSBIPutStr("exec:      SysBase @ ");
            krnSBIPutHex((uint64_t)(uintptr_t)SysBase);
            krnSBIPutStr("\n[boot] InitCode(RTF_SINGLETASK)\n");
            InitCode(RTF_SINGLETASK, 0);

            krnSBIPutStr("[boot] InitCode(RTF_COLDSTART)\n");
            InitCode(RTF_COLDSTART, 0);

            /*
             * With only kernel.resource, exec.library and task.resource
             * in the kickstart there is nothing (like dos.library) to
             * take over the machine, so InitCode returns. Prove exec is
             * functional with a smoke test through the LVO table.
             */
            {
                struct Task *me = FindTask(NULL);
                APTR mem;

                krnSBIPutStr("exec:      ThisTask = ");
                if (me && me->tc_Node.ln_Name)
                    krnSBIPutStr(me->tc_Node.ln_Name);
                else
                    krnSBIPutStr("(unnamed)");
                krnSBIPutStr("\n");

                mem = AllocMem(64 << 10, MEMF_ANY | MEMF_CLEAR);
                krnSBIPutStr("exec:      AllocMem(64K) = ");
                krnSBIPutHex((uint64_t)(uintptr_t)mem);
                krnSBIPutStr("\n");
                if (mem)
                    FreeMem(mem, 64 << 10);

                krnSBIPutStr("exec:      AvailMem(MEMF_ANY) = ");
                krnSBIPutDec(AvailMem(MEMF_ANY) >> 20);
                krnSBIPutStr(" MiB free\n");
            }

            /* "testsched" on the command line: prove a second task can
               run and signal us back */
            if (fdtinfo.bootargs)
            {
                const char *s;
                for (s = fdtinfo.bootargs; *s; s++)
                {
                    if (s[0] == 't' && s[1] == 'e' && s[2] == 's' &&
                        s[3] == 't' && s[4] == 's' && s[5] == 'c' &&
                        s[6] == 'h' && s[7] == 'e' && s[8] == 'd')
                    {
                        krnSchedTest();
                        break;
                    }
                }
            }
        }
        else
            krnSBIPutStr("[boot] PANIC: krnPrepareExecBase failed!\n");
    }

    krnSBIPutStr("early startup complete - halting.\n");

    for (;;)
        asm volatile("wfi");
}
