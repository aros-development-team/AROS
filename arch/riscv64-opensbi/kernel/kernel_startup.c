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
#include <hardware/vbe.h>
#include <asm/cpu.h>
#include <asm/riscv64/mmu.h>
#include <proto/exec.h>

#include "kernel_sbi.h"
#include "kernel_intern.h"
#include "kernel_romtags.h"
#include "tlsf.h"

/* rom/kernel/kernel_memory.c provides no header for this */
void krnCreateMemHeader(CONST_STRPTR name, BYTE pri, APTR start, IPTR size,
                        ULONG flags);

/* The hart we were booted on (see kernel_intern.h) */
unsigned long __boot_hartid;
/* Harts described by the device tree; only the boot hart runs AROS
   until SMP bring-up exists, but the count is real */
unsigned long __ncpus = 1;
int __boot_cpu_index;

/* kernel.resource publishes the boot tags through KrnGetBootInfo() */
extern struct TagItem *BootMsg;

/* Set by the UEFI stub when it loaded the BSP package itself */
extern unsigned long __efi_initrd_start, __efi_initrd_end;
extern unsigned long __efi_mem_start, __efi_mem_size;
extern unsigned long __efi_nregions, __efi_regions[][2];
extern unsigned long __efi_systab;
extern char __efi_cmdline[];
extern unsigned long __efi_fb_base, __efi_fb_size;
extern unsigned int __efi_fb_width, __efi_fb_height, __efi_fb_pitch;
extern unsigned int __efi_fb_bpp;
extern unsigned int __efi_fb_redmask, __efi_fb_greenmask;
extern unsigned int __efi_fb_bluemask, __efi_fb_resmask;

struct ExecBase *SysBase __attribute__((section(".data"))) = NULL;

/* Linker script symbols delimiting the kernel image */
extern char __text_start[];
extern char __kernel_end[];

/* Lowest allocatable memory answers MEMF_CHIP requests */
#define CHIPMEM_SIZE    (16 * 1024 * 1024)

/*
 * RAM here begins at 2GB, so exec's below-2GB rule never marks a bank
 * MEMF_31BIT - yet hunk relocation (AmigaOS font and keymap files) and
 * 32-bit DMA bounce buffers all allocate with that flag, and without a
 * bank carrying it every such allocation fails outright. Any bank that
 * stays fully below 4GB satisfies what those consumers actually need
 * (32-bit addressability), so declare the flag on those banks.
 * "end" is the exclusive upper bound of the bank.
 */
#define MEMF_BELOW4G(end) \
    ((((IPTR)(end) - 1) <= 0xFFFFFFFFUL) ? MEMF_31BIT : 0)

#define BOOTTAG_MAX 16
static struct TagItem BootTags[BOOTTAG_MAX];

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

/*
 * Is word present in the command line? Matched on whole words so that
 * "dumpdt" does not also fire on "nodumpdt".
 */
static int krnArgMatch(const char *args, const char *word)
{
    const char *s;

    for (s = args; *s; s++)
    {
        const char *a = s, *w = word;

        if (s != args && s[-1] != ' ')
            continue;
        while (*w && *a == *w)
        {
            a++;
            w++;
        }
        if (!*w && (!*a || *a == ' '))
            return 1;
    }
    return 0;
}

/*
 * Every romtag in a range, as exec's scanner will see it.
 *
 * Modules are relocated into place by kernel_elf.c, so a romtag is
 * only found if its self-pointer was relocated correctly - and the
 * scanner jumps forward by rt_EndSkip after each hit, so one bad value
 * silently swallows the modules that follow. Printing what the same
 * walk finds is the quickest way to tell a module that failed to
 * initialise from one that was never looked at.
 */
static void krnDumpResidents(UWORD *lo, UWORD *hi)
{
    UWORD *ptr = lo;

    krnSBIPutStr("--- residents ---\n");

    while (ptr < hi)
    {
        struct Resident *res = (struct Resident *)ptr;

        if (res->rt_MatchWord == RTC_MATCHWORD && res->rt_MatchTag == res)
        {
            krnSBIPutStr("  ");
            krnSBIPutHex((unsigned long)res);
            krnSBIPutStr(" pri ");
            krnSBIPutDec((unsigned char)res->rt_Pri);
            krnSBIPutStr(" flags ");
            krnSBIPutHex32(res->rt_Flags);
            krnSBIPutStr(" skip ");
            krnSBIPutHex((unsigned long)res->rt_EndSkip);
            krnSBIPutStr(" ");
            krnSBIPutStr((const char *)res->rt_Name);
            krnSBIPutStr("\n");

            if ((IPTR)res->rt_EndSkip > (IPTR)ptr)
                ptr = (UWORD *)res->rt_EndSkip - 2;
            if ((IPTR)ptr & 1)
                ptr = (UWORD *)((IPTR)ptr + 1);
        }
        ptr++;
    }

    krnSBIPutStr("--- end residents ---\n");
}

/*
 * The framebuffer description the UEFI stub captured from the GOP,
 * recast as the VBE mode record every consumer of KRN_VBEModeInfo
 * (bootloader.resource, and the graphics driver through it) already
 * understands. The same translation the pc bootstrap does for the
 * multiboot2 framebuffer tag (arch/all-pc/bootstrap/multiboot2.c).
 */
static struct vbe_mode VBEModeInfo;

/*
 * Sv39 can only identity-map the lower 256GB: a virtual address with
 * bit 38 set has to be sign-extended, so a physical address up there
 * can never be its own virtual address - the Titan's GOP framebuffer
 * sits in a PCIe window at 256GB exactly. Such a framebuffer is
 * remapped into a fixed window well above every RAM bank, and the
 * remapped address is what goes into the boot tags; every consumer
 * just writes through the pointer and never needs the physical one.
 */
#define SV39_IDENTITY_LIMIT (1UL << 38)
#define FB_WINDOW           0x2000000000UL

static IPTR krnFBAddr(void)
{
    if (__efi_fb_base + __efi_fb_size <= SV39_IDENTITY_LIMIT)
        return __efi_fb_base;
    return FB_WINDOW | (__efi_fb_base & (MEGAPAGE_SIZE - 1));
}

static void krnFBMask(unsigned int mask,
                      unsigned char *size, unsigned char *pos)
{
    unsigned char s = 0, p = 0;

    if (mask)
    {
        while (!(mask & 1))
        {
            mask >>= 1;
            p++;
        }
        while (mask & 1)
        {
            mask >>= 1;
            s++;
        }
    }
    *size = s;
    *pos  = p;
}

static void krnFillVBEFromGOP(void)
{
    VBEModeInfo.mode_attributes = VM_SUPPORTED | VM_COLOR | VM_GRAPHICS |
                                  VM_NO_VGA_HW | VM_NO_VGA_MEM | VM_LINEAR_FB;
    VBEModeInfo.bytes_per_scanline = __efi_fb_pitch;
    VBEModeInfo.x_resolution       = __efi_fb_width;
    VBEModeInfo.y_resolution       = __efi_fb_height;
    VBEModeInfo.bits_per_pixel     = __efi_fb_bpp;
    VBEModeInfo.number_of_planes   = 1;
    VBEModeInfo.memory_model       = VMEM_RGB;

    krnFBMask(__efi_fb_redmask, &VBEModeInfo.red_mask_size,
              &VBEModeInfo.red_field_position);
    krnFBMask(__efi_fb_greenmask, &VBEModeInfo.green_mask_size,
              &VBEModeInfo.green_field_position);
    krnFBMask(__efi_fb_bluemask, &VBEModeInfo.blue_mask_size,
              &VBEModeInfo.blue_field_position);
    krnFBMask(__efi_fb_resmask, &VBEModeInfo.reserved_mask_size,
              &VBEModeInfo.reserved_field_position);

    /* Truncates above 4GB; KRN_FBAddr carries the full address */
    VBEModeInfo.phys_base = krnFBAddr();

    VBEModeInfo.linear_bytes_per_scanline      = VBEModeInfo.bytes_per_scanline;
    VBEModeInfo.linear_red_mask_size           = VBEModeInfo.red_mask_size;
    VBEModeInfo.linear_red_field_position      = VBEModeInfo.red_field_position;
    VBEModeInfo.linear_green_mask_size         = VBEModeInfo.green_mask_size;
    VBEModeInfo.linear_green_field_position    = VBEModeInfo.green_field_position;
    VBEModeInfo.linear_blue_mask_size          = VBEModeInfo.blue_mask_size;
    VBEModeInfo.linear_blue_field_position     = VBEModeInfo.blue_field_position;
    VBEModeInfo.linear_reserved_mask_size      = VBEModeInfo.reserved_mask_size;
    VBEModeInfo.linear_reserved_field_position = VBEModeInfo.reserved_field_position;
}

struct TagItem *krnPrepareBootTags(void *fdt, struct krnFDTInfo *info)
{
    struct TagItem *tag = BootTags;

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
    /* Filled in once the module package has been loaded */
    tag->ti_Tag  = KRN_DebugInfo;
    tag->ti_Data = 0;
    tag++;
    if (info->bootargs)
    {
        tag->ti_Tag  = KRN_CmdLine;
        tag->ti_Data = (IPTR)info->bootargs;
        tag++;
    }
    /* Booted through UEFI: efi.resource hands this to acpica.library,
       which finds the ACPI root pointer in its configuration table */
    if (__efi_systab)
    {
        tag->ti_Tag  = KRN_EFISystemTable;
        tag->ti_Data = (IPTR)__efi_systab;
        tag++;
    }
    /* The firmware left a framebuffer on display - describe it the way
       the pc ports do, so bootloader.resource publishes it as BL_Video */
    if (__efi_fb_base)
    {
        krnFillVBEFromGOP();
        tag->ti_Tag  = KRN_VBEModeInfo;
        tag->ti_Data = (IPTR)&VBEModeInfo;
        tag++;
        tag->ti_Tag  = KRN_FBAddr;
        tag->ti_Data = krnFBAddr();
        tag++;
    }
    tag->ti_Tag  = TAG_DONE;
    tag->ti_Data = 0;

    return BootTags;
}

/* Kept static so the platform init can read it once exec is up */
static struct krnFDTInfo fdtinfo;
struct krnFDTInfo *__bootfdtinfo = &fdtinfo;

void __attribute__((noreturn)) kernel_cstart(unsigned long hartid, void *fdt)
{
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
        if (fdtinfo.nregions > 1)
        {
            unsigned int r;
            for (r = 0; r < fdtinfo.nregions; r++)
            {
                krnSBIPutStr("  bank:    ");
                krnSBIPutHex(fdtinfo.regions[r].base);
                krnSBIPutStr(" - ");
                krnSBIPutHex(fdtinfo.regions[r].base + fdtinfo.regions[r].size);
                krnSBIPutStr("\n");
            }
        }
        if (fdtinfo.bootargs)
        {
            krnSBIPutStr("bootargs:  ");
            krnSBIPutStr(fdtinfo.bootargs);
            krnSBIPutStr("\n");
        }
    }
    else if (__efi_mem_size)
    {
        /* Booted through UEFI on a machine described by ACPI rather
           than a device tree - the stub handed us the memory map */
        krnSBIPutStr("memory:    (from EFI) ");
        krnSBIPutHex(__efi_mem_start);
        krnSBIPutStr(" - ");
        krnSBIPutHex(__efi_mem_start + __efi_mem_size);
        krnSBIPutStr("\n");
        fdtinfo.mem_base = __efi_mem_start;
        fdtinfo.mem_size = __efi_mem_size;
        fdtinfo.bootargs = __efi_cmdline;
        fdtinfo.ncpus    = 1;
        fdtinfo.tb_freq  = 10000000;    /* the common default */
        fdtinfo.initrd_start = fdtinfo.initrd_end = 0;
    }
    else
    {
        krnSBIPutStr("[boot] WARNING: could not parse the device tree!\n");
        fdtinfo.mem_base = 0x80000000UL;
        fdtinfo.mem_size = 128 << 20;
    }

    if (fdtinfo.ncpus)
        __ncpus = fdtinfo.ncpus;
    __boot_cpu_index = fdtinfo.boot_cpu;


    /*
     * On a UEFI boot the firmware's memory map is the authoritative
     * description of RAM - the device tree handed over can describe
     * less than is actually fitted. Take the
     * EFI regions in preference when we have them.
     */
    if (__efi_nregions)
    {
        unsigned int r;

        fdtinfo.nregions = 0;
        for (r = 0; r < __efi_nregions && r < KRN_MAX_MEMREGIONS; r++)
        {
            fdtinfo.regions[r].base = __efi_regions[r][0];
            fdtinfo.regions[r].size = __efi_regions[r][1];
            fdtinfo.nregions++;
            if (__efi_regions[r][1] > fdtinfo.mem_size)
            {
                fdtinfo.mem_base = __efi_regions[r][0];
                fdtinfo.mem_size = __efi_regions[r][1];
            }
        }
        krnSBIPutStr("memory:    (EFI map) ");
        krnSBIPutDec(fdtinfo.nregions);
        krnSBIPutStr(" region(s)\n");
        for (r = 0; r < fdtinfo.nregions; r++)
        {
            krnSBIPutStr("  bank:    ");
            krnSBIPutHex(fdtinfo.regions[r].base);
            krnSBIPutStr(" - ");
            krnSBIPutHex(fdtinfo.regions[r].base + fdtinfo.regions[r].size);
            krnSBIPutStr("\n");
        }
    }

    /*
     * A device tree need not carry a command line, and most do not.
     * Fall back to whatever the UEFI stub was given - its load options,
     * or aros.cmd on the boot volume.
     */
    if (!fdtinfo.bootargs && __efi_cmdline[0])
    {
        fdtinfo.bootargs = __efi_cmdline;
        krnSBIPutStr("bootargs:  ");
        krnSBIPutStr(fdtinfo.bootargs);
        krnSBIPutStr(" (default)\n");
    }

    /*
     * The UEFI stub loads the package itself when no loader placed one
     * for us. This has to be known before the MMU is set up, because
     * the firmware can put it outside every region the device tree
     * describes (it can land in a bank the device tree omits, while the
     * device tree only reports memory up to 18GiB).
     */
    if (!fdtinfo.initrd_start && __efi_initrd_end > __efi_initrd_start)
    {
        fdtinfo.initrd_start = __efi_initrd_start;
        fdtinfo.initrd_end   = __efi_initrd_end;
    }

    /*
     * "dumpdt" / "dumpacpi" on the command line: show what the firmware
     * describes. Both run before the MMU is enabled, while every table
     * the firmware left behind is still reachable by its address.
     */
    if (fdtinfo.bootargs)
    {
        if (krnArgMatch(fdtinfo.bootargs, "dumpdt"))
            krnDumpFDT(fdt);
        if (krnArgMatch(fdtinfo.bootargs, "dumpacpi"))
            krnDumpACPI();
    }

    /*
     * Build the boot tags only now: the memory layout and the command
     * line are settled above, and kernel.resource publishes this list
     * to everything that asks (bootloader.resource reads the command
     * line from it).
     */
    msg = krnPrepareBootTags(fdt, &fdtinfo);
    BootMsg = msg;
    krnSBIPutStr("boot tags prepared.\n");

    krnInitMMU(&fdtinfo);

    /*
     * The interrupt controller, now that its registers can be mapped.
     * The device tree is asked first and stays authoritative; a machine
     * that handed over no tree is described by the MADT instead.
     */
    if (!fdtinfo.plic_base)
        krnACPIPLICInfo(&fdtinfo);
    krnPLICInit(&fdtinfo);

    /*
     * The package (and the device tree itself) can live in memory the
     * firmware owns and the device tree never lists - the
     * loader placed it in a bank above 32GiB. Make sure both are
     * reachable before anything touches them.
     */
    if (fdtinfo.initrd_start && fdtinfo.initrd_end > fdtinfo.initrd_start)
    {
        krnMMUMapRegion(fdtinfo.initrd_start,
                        fdtinfo.initrd_end - fdtinfo.initrd_start,
                        PTE_R | PTE_W | PTE_X);
        krnSBIPutStr("mmu:       mapped package ");
        krnSBIPutHex(fdtinfo.initrd_start);
        krnSBIPutStr(" - ");
        krnSBIPutHex(fdtinfo.initrd_end);
        krnSBIPutStr("\n");
    }
    if (fdt)
        krnMMUMapRegion((IPTR)fdt, 2 << 20, PTE_R | PTE_W);

    /*
     * The firmware framebuffer sits in the display hardware's memory,
     * outside every RAM bank the maps above cover. The graphics driver
     * writes to it by address, so it has to be reachable.
     */
    if (__efi_fb_base && __efi_fb_size)
    {
        IPTR fbva = krnFBAddr();

        if (fbva == __efi_fb_base)
            krnMMUMapRegion(__efi_fb_base, __efi_fb_size, PTE_R | PTE_W);
        else
            krnMMUMapPages(fbva, __efi_fb_base, __efi_fb_size,
                           PTE_R | PTE_W);
        krnSBIPutStr("gfx:       ");
        krnSBIPutDec(__efi_fb_width);
        krnSBIPutStr("x");
        krnSBIPutDec(__efi_fb_height);
        krnSBIPutStr("x");
        krnSBIPutDec(__efi_fb_bpp);
        krnSBIPutStr(" framebuffer @ ");
        krnSBIPutHex(__efi_fb_base);
        krnSBIPutStr(" size ");
        krnSBIPutHex(__efi_fb_size);
        if (fbva != __efi_fb_base)
        {
            krnSBIPutStr(" mapped @ ");
            krnSBIPutHex(fbva);
        }
        krnSBIPutStr("\n");
    }

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
        struct MemHeader *mh, *chipmh;
        IPTR chiplow = 0, bootlow;
        BOOL use_tlsf;
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

                /* Now the module descriptor chain exists */
                {
                    struct TagItem *tag;

                    for (tag = BootTags; tag->ti_Tag != TAG_DONE; tag++)
                    {
                        if (tag->ti_Tag == KRN_DebugInfo)
                        {
                            tag->ti_Data = (IPTR)__ks_debuginfo;
                            break;
                        }
                    }
                }
            }
            else
                krnSBIPutStr("[boot] WARNING: no modules loaded!\n");
        }

        /*
         * MEMF_CHIP has to be satisfiable by something, or every caller
         * asking for it - sprite and audio buffers among them - gets
         * NULL with nothing to say why. There is no separate chip bus
         * here and all memory is equally reachable, so the lowest 16MB
         * answers for it. It is given a lower priority than the rest so
         * that ordinary allocations drain the fast pool first and leave
         * it for those that actually need it.
         */
        /*
         * Use the TLSF allocator for all memory headers, like the other
         * modern ports; "notlsf" on the command line falls back to exec's
         * classic first-fit lists. Note krnConvertMemHeaderToTLSF() places
         * the new header at the first free chunk, so the returned pointer
         * is not the region base - the bases are kept separately for the
         * bank-exclusion arithmetic below.
         */
        use_tlsf = !(fdtinfo.bootargs &&
                     krnArgMatch(fdtinfo.bootargs, "notlsf"));
        if (use_tlsf)
            krnSBIPutStr("mem:       TLSF allocator enabled\n");

        chipmh = NULL;
        if (memhigh - memlow > CHIPMEM_SIZE * 2)
        {
            chiplow = memlow;
            chipmh = (struct MemHeader *)memlow;
            krnCreateMemHeader("Chip Memory", -6, (APTR)memlow, CHIPMEM_SIZE,
                               MEMF_CHIP | MEMF_PUBLIC | MEMF_KICK |
                               MEMF_LOCAL |
                               MEMF_BELOW4G(memlow + CHIPMEM_SIZE));
            if (use_tlsf)
            {
                struct MemHeader *tmh = krnConvertMemHeaderToTLSF(chipmh);
                if (tmh)
                    chipmh = tmh;
            }
            memlow += CHIPMEM_SIZE;
        }

        bootlow = memlow;
        mh = (struct MemHeader *)memlow;
        krnCreateMemHeader("System Memory", 0, (APTR)memlow,
                           memhigh - memlow,
                           MEMF_FAST | MEMF_PUBLIC | MEMF_KICK | MEMF_LOCAL |
                           MEMF_BELOW4G(memhigh));
        if (use_tlsf)
        {
            struct MemHeader *tmh = krnConvertMemHeaderToTLSF(mh);
            if (tmh)
                mh = tmh;
            else
                use_tlsf = FALSE;   /* stay consistent for the banks below */
        }

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

        if (fdtinfo.bootargs && krnArgMatch(fdtinfo.bootargs, "dumpres"))
            krnDumpResidents((UWORD *)__text_start, (UWORD *)__kernel_end);
        if (fdtinfo.bootargs && krnArgMatch(fdtinfo.bootargs, "dumpres") &&
            modhigh > modlow)
            krnDumpResidents((UWORD *)modlow, (UWORD *)modhigh);

        if (krnPrepareExecBase(ranges, mh, msg))
        {
            /* Exec is up and owns a MemList now, so chip can join it */
            if (chipmh)
                Enqueue(&SysBase->MemList, &chipmh->mh_Node);

            /*
             * The bootstrap heap above is only the slice of one bank
             * below the DTB - enough to bring exec up, but a real
             * machine has more banks and tens of GiB more in this one.
             * Every firmware region is already mapped; now exec can
             * hold further MemHeaders, add each usable span that the
             * bootstrap heap, the chip pool, the kernel image, the boot
             * package and the DTB do not already cover.
             */
            {
                IPTR ex_lo[8], ex_hi[8];
                IPTR kern_hi = ((IPTR)__kernel_end + 4095) & ~(IPTR)4095;
                int nx = 0, r, i, j;

                /* Spans already in use, that a new header must not cover.
                   Anything below the kernel - where the boot/exception
                   stack lives and grows down - is kept out by clamping
                   each region's start to kern_hi below, not listed here. */
                ex_lo[nx] = bootlow;        ex_hi[nx] = memhigh;              nx++;
                if (chipmh) { ex_lo[nx] = chiplow;
                              ex_hi[nx] = chiplow + CHIPMEM_SIZE;             nx++; }
                if (modhigh > modlow) {
                    ex_lo[nx] = modlow & ~(IPTR)4095;
                    ex_hi[nx] = (modhigh + 4095) & ~(IPTR)4095;              nx++; }
                /* The boot package image is still where the loader left
                   it. Its modules were relocated out, but keep it whole
                   until boot is done rather than let it be handed out. */
                if (fdtinfo.initrd_start &&
                    fdtinfo.initrd_end > fdtinfo.initrd_start) {
                    ex_lo[nx] = fdtinfo.initrd_start & ~(IPTR)4095;
                    ex_hi[nx] = (fdtinfo.initrd_end + 4095) & ~(IPTR)4095;   nx++; }
                if (dtbaddr) {
                    ex_lo[nx] = dtbaddr & ~(IPTR)4095;
                    ex_hi[nx] = (dtbaddr + (2 << 20) + 4095) & ~(IPTR)4095;  nx++; }

                /* Sort the exclusions by start so one pass can subtract
                   them from each region. */
                for (i = 1; i < nx; i++) {
                    IPTR a = ex_lo[i], b = ex_hi[i];
                    for (j = i; j > 0 && ex_lo[j-1] > a; j--) {
                        ex_lo[j] = ex_lo[j-1]; ex_hi[j] = ex_hi[j-1];
                    }
                    ex_lo[j] = a; ex_hi[j] = b;
                }

                for (r = 0; r < (int)fdtinfo.nregions; r++) {
                    IPTR rs = (fdtinfo.regions[r].base + 4095) & ~(IPTR)4095;
                    IPTR re = (fdtinfo.regions[r].base +
                               fdtinfo.regions[r].size) & ~(IPTR)4095;
                    /* Never hand out anything below the kernel */
                    IPTR p = (rs < kern_hi) ? kern_hi : rs;

                    for (i = 0; i <= nx; i++) {
                        IPTR elo = (i < nx) ? ex_lo[i] : re;
                        IPTR ehi = (i < nx) ? ex_hi[i] : re;
                        IPTR glo = p, ghi = (elo < re) ? elo : re;

                        if (ghi > glo && (ghi - glo) >= (1 << 20)) {
                            struct MemHeader *xmh = (struct MemHeader *)glo;
                            krnCreateMemHeader("System Memory", 0, (APTR)glo,
                                               ghi - glo, MEMF_FAST |
                                               MEMF_PUBLIC | MEMF_KICK |
                                               MEMF_LOCAL |
                                               MEMF_BELOW4G(ghi));
                            if (use_tlsf) {
                                struct MemHeader *tmh =
                                    krnConvertMemHeaderToTLSF(xmh);
                                if (tmh)
                                    xmh = tmh;
                            }
                            Enqueue(&SysBase->MemList, &xmh->mh_Node);
                            krnSBIPutStr("mem:       added bank ");
                            krnSBIPutHex(glo);
                            krnSBIPutStr(" - ");
                            krnSBIPutHex(ghi);
                            krnSBIPutStr("\n");
                        }
                        if (i < nx && ehi > p) p = ehi;
                        if (p >= re) break;
                    }
                }
            }

            /* Diagnostic: dump Exec's actual MemHeader list (name, range,
             * free bytes and mh_Attributes flags) so the free pool and the
             * MEMF_* flags of every bank can be verified. Temporary. */
            {
                struct MemHeader *dmh;
                krnSBIPutStr("mem: exec MemList (name lower - upper"
                             " free attr):\n");
                for (dmh = (struct MemHeader *)SysBase->MemList.lh_Head;
                     dmh->mh_Node.ln_Succ;
                     dmh = (struct MemHeader *)dmh->mh_Node.ln_Succ) {
                    krnSBIPutStr("  MH '");
                    krnSBIPutStr(dmh->mh_Node.ln_Name ?
                                 dmh->mh_Node.ln_Name : "?");
                    krnSBIPutStr("' ");
                    krnSBIPutHex((IPTR)dmh->mh_Lower);
                    krnSBIPutStr(" - ");
                    krnSBIPutHex((IPTR)dmh->mh_Upper);
                    krnSBIPutStr(" free=");
                    krnSBIPutHex((IPTR)dmh->mh_Free);
                    krnSBIPutStr(" attr=");
                    krnSBIPutHex((IPTR)dmh->mh_Attributes);
                    krnSBIPutStr("\n");
                }
            }

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
