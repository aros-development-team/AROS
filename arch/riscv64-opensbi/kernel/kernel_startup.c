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
#include <asm/riscv64/mmu.h>
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

/* "testfpu" support: FP computation interleaved across two tasks */
static ULONG fpuTaskSig;
static volatile int fpuTaskOk = -1;

static double fpuChunk(double v, double mul, double add)
{
    int i;
    for (i = 0; i < 1000; i++)
        v = v * mul + add;
    return v;
}

static void fpuTestEntry(void)
{
    double v = 1.0, check = 1.0;
    BYTE sigbit = AllocSignal(-1);
    int round;

    fpuTaskSig = 1UL << sigbit;
    Signal(bootTask, schedSig);          /* ready */

    for (round = 0; round < 4; round++)
    {
        Wait(fpuTaskSig);
        v = fpuChunk(v, 1.0009765625, 0.03125);
        Signal(bootTask, schedSig);
    }

    /* Local recompute - must match the interleaved result exactly */
    for (round = 0; round < 4; round++)
        check = fpuChunk(check, 1.0009765625, 0.03125);

    /* Publish the verdict; the boot task polls it (a further Signal
       would merge with the final round's still-pending bit) */
    fpuTaskOk = (v == check);
    Wait(0);
}

static void krnFpuTest(void)
{
    struct TagItem tags[] =
    {
        { TASKTAG_NAME, (IPTR)"FpuTest"        },
        { TASKTAG_PRI,  5                      },
        { TASKTAG_PC,   (IPTR)fpuTestEntry     },
        { TAG_DONE,     0                      }
    };
    struct Task *t;
    double v = 2.0, check = 2.0;
    BYTE sigbit;
    int round, bootOk;

    bootTask = FindTask(NULL);
    sigbit = AllocSignal(-1);
    schedSig = 1UL << sigbit;

    krnSBIPutStr("[testfpu] interleaving FP work across two tasks...\n");
    t = NewCreateTaskA(tags);
    if (!t)
    {
        krnSBIPutStr("[testfpu] FAILED to create the task!\n");
        return;
    }
    Wait(schedSig);                      /* task ready, fpuTaskSig set */

    for (round = 0; round < 4; round++)
    {
        Signal(t, fpuTaskSig);
        v = fpuChunk(v, 0.9990234375, 0.0625);
        Wait(schedSig);
    }
    /* The higher-priority task owns the CPU until it publishes its
       verdict and sleeps; poll for it */
    while (fpuTaskOk < 0)
        asm volatile("" ::: "memory");

    for (round = 0; round < 4; round++)
        check = fpuChunk(check, 0.9990234375, 0.0625);
    bootOk = (v == check);

    if (bootOk && fpuTaskOk == 1)
        krnSBIPutStr("[testfpu] PASS - FPU state preserved across switches!\n");
    else
    {
        krnSBIPutStr("[testfpu] FAIL: boot ");
        krnSBIPutDec(bootOk);
        krnSBIPutStr(" task ");
        krnSBIPutDec(fpuTaskOk);
        krnSBIPutStr("\n");
    }
    FreeSignal(sigbit);
}

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
        UWORD *ranges[5];
        IPTR modlow = 0, modhigh = 0;
        IPTR memlow  = ((IPTR)__kernel_end + 4095) & ~(IPTR)4095;
        IPTR memhigh = fdtinfo.mem_base + fdtinfo.mem_size;
        IPTR dtbaddr = (IPTR)fdt;

        /* Keep the DTB (placed near the top of RAM by qemu/OpenSBI)
           out of the allocatable pool */
        if (dtbaddr >= memlow && dtbaddr < memhigh)
            memhigh = dtbaddr & ~(IPTR)4095;

        /*
         * Load any boot modules delivered alongside the kickstart (a
         * package placed in RAM by the loader - qemu -initrd, U-Boot
         * or UEFI - and pointed at by /chosen). They are placed right
         * after the kernel and become a second romtag scan range.
         */
        if (fdtinfo.initrd_start && fdtinfo.initrd_end > fdtinfo.initrd_start)
        {
            IPTR lo = 0, hi = 0, used = memlow;
            IPTR isize = fdtinfo.initrd_end - fdtinfo.initrd_start;
            int n;

            krnSBIPutStr("modules:   package @ ");
            krnSBIPutHex(fdtinfo.initrd_start);
            krnSBIPutStr(" (");
            krnSBIPutDec(isize >> 10);
            krnSBIPutStr(" KiB)\n");

            /* Keep the package itself out of the allocatable pool */
            if (fdtinfo.initrd_start >= memlow &&
                fdtinfo.initrd_start < (IPTR)memhigh)
                memhigh = fdtinfo.initrd_start & ~(IPTR)4095;

            n = krnLoadPackage((void *)(IPTR)fdtinfo.initrd_start, isize,
                               memlow, memhigh, &lo, &hi, &used);
            if (n > 0)
            {
                krnSBIPutStr("modules:   ");
                krnSBIPutDec(n);
                krnSBIPutStr(" loaded, ");
                krnSBIPutHex(lo);
                krnSBIPutStr(" - ");
                krnSBIPutHex(hi);
                krnSBIPutStr("\n");
                /* The modules landed in what the initial map treats as
                   data - their code has to be executable */
                krnMMUSetPerms(lo, (hi + 4095) & ~(IPTR)4095,
                               PTE_R | PTE_W | PTE_X);
                modlow  = lo;
                modhigh = hi;
                memlow  = (used + 4095) & ~(IPTR)4095;
            }
            else
                krnSBIPutStr("[boot] WARNING: no modules loaded!\n");
        }

        mh = (struct MemHeader *)memlow;
        krnCreateMemHeader("System Memory", 0, (APTR)memlow,
                           memhigh - memlow,
                           MEMF_FAST | MEMF_PUBLIC | MEMF_KICK | MEMF_LOCAL);

        ranges[0] = (UWORD *)__text_start;
        ranges[1] = (UWORD *)__kernel_end;
        if (modhigh)
        {
            ranges[2] = (UWORD *)modlow;
            ranges[3] = (UWORD *)modhigh;
            ranges[4] = (UWORD *)-1;
        }
        else
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

                /* Prove a package-loaded module is usable */
                {
                    struct Library *ub = OpenLibrary("utility.library", 0);
                    krnSBIPutStr("modules:   OpenLibrary(utility) = ");
                    krnSBIPutHex((uint64_t)(uintptr_t)ub);
                    if (ub)
                    {
                        krnSBIPutStr(" v");
                        krnSBIPutDec(ub->lib_Version);
                        CloseLibrary(ub);
                    }
                    krnSBIPutStr("\n");
                }
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
                    }
                    if (s[0] == 't' && s[1] == 'e' && s[2] == 's' &&
                        s[3] == 't' && s[4] == 'f' && s[5] == 'p' &&
                        s[6] == 'u')
                    {
                        krnFpuTest();
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
