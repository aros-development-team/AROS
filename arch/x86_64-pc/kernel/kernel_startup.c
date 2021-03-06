/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.
*/

#include <aros/multiboot.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <aros/symbolsets.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <utility/tagitem.h>
#include <proto/arossupport.h>
#include <proto/exec.h>

#include <bootconsole.h>
#include <inttypes.h>
#include <string.h>

#include "boot_utils.h"
#include "kernel_base.h"
#include "kernel_intern.h"
#include "kernel_bootmem.h"
#include "kernel_debug.h"
#include "kernel_mmap.h"
#include "kernel_romtags.h"
#include "smp.h"
#include "tls.h"

#define D(x)
#define DSTACK(x)

static APTR core_AllocBootTLS(struct KernBootPrivate *);
static APTR core_AllocBootTSS(struct KernBootPrivate *);
static APTR core_AllocBootGDT(struct KernBootPrivate *);
static APTR core_AllocBootIDT(struct KernBootPrivate *);

/* Common IBM PC memory layout (64bit version) */
static const struct MemRegion PC_Memory[] =
{
    /*
     * Low memory has a bit lower priority -:
     * - This helps the kernel/exec locate its MemHeader.
     * - We explicitly need low memory for SMP bootstrap.
     */
    {0x000000000, 0x000100000, "Low memory"    , -6, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_CHIP|MEMF_31BIT|MEMF_24BITDMA},
    {0x000100000, 0x001000000, "ISA DMA memory", -5, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_CHIP|MEMF_31BIT|MEMF_24BITDMA},
    /*
     * 1. 64-bit machines can expose RAM at addresses up to 0xD0000000 (giving 3.5 GB total).
     *    All MMIO sits beyond this border. AROS intentionally specifies a 4GB limit, in case some
     *    devices expose even more RAM in this space. This allows all the RAM to be usable.
     * 2. AROS has MEMF_31BIT (compatable with MorphOS). This has likely originated from the assumption
     *    that MMIO starts at 0x80000000 (which is true at least for PegasosPPC), though on AROS it
     *    is used to ensure memory allocations lie within the low 32bit address space.
     */
    {0x001000000, 0x080000000, "31-bit memory" ,  0, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_CHIP|MEMF_31BIT              },
    {0x080000000, -1,          "High memory"   , 10, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_FAST                         },
    {0          , 0          , NULL            ,  0, 0                                                                  }
};

static ULONG allocator = ALLOCATOR_TLSF;

/*
 * Boot-time global variables.
 * __KernBootPrivate needs to survive accross warm reboots, so it's put into .data.
 * SysBase is intentionally put into .rodata. This way we prevent it from being modified.
 */
__attribute__((section(".data"))) struct KernBootPrivate *__KernBootPrivate = NULL;
__attribute__((section(".data"))) IPTR kick_highest = 0;
__attribute__((section(".rodata"))) struct ExecBase *SysBase = NULL;

static void boot_start(struct TagItem *msg);
static char boot_stack[];

/*
 * This is where our kernel started.
 * First we clear BSS section, then switch stack pointer to our temporary stack
 * (which is itself located in BSS). While we are here, the stack is actually
 * located inside our bootstrap, and it's safe to use it a little bit.
 */
IPTR __startup start64(struct TagItem *msg, ULONG magic)
{
    /* Anti-command-line-run protector */
    if (magic == AROS_BOOT_MAGIC)
    {
        /* Run the kickstart from boot_start() routine. */
        core_Kick(msg, boot_start);
    }

    return -1;
}

/*
 * This code is executed only once, after the kickstart is loaded by bootstrap.
 * Its main job is to initialize early debugging console ASAP in order to be able
 * to see what happens. This will deal with both serial and on-screen console.
 *
 * Console mirror is placed at the end of bootstrap's protected area. We must not
 * overwrite it because it contains boot-time GDT, taglist, and some other structures.
 *
 * Default address is bootstrap start + 4KB, just in case.
 */
static void boot_start(struct TagItem *msg)
{
    fb_Mirror = (void *)LibGetTagData(KRN_ProtAreaEnd, 0x101000, msg);
    con_InitTagList(msg);

    bug("AROS64 - The AROS Research OS\n");
    bug("64-bit ");
#if defined(__AROSEXEC_SMP__)
    bug("SMP ");
#endif
    bug("build. Compiled %s\n", __DATE__);
    D(bug("[Kernel] boot_start: Jumped into kernel.resource @ %p [stub @ %p].\n", boot_start, start64));

    kernel_cstart(msg);
}

/*
 * This routine actually launches the kickstart. It's called either upon first start or upon warm reboot.
 * The only assumption is that stack is outside .bss . For both cases this is true:
 * 1. First boot - the stack is located inside the bootstrap.
 * 2. Warm reboot - the stack is located in supervisor area (__KernBootPrivate->SystemStack).
 */
void core_Kick(struct TagItem *msg, void *target)
{
    const struct TagItem *bss = LibFindTagItem(KRN_KernelBss, msg);

    /* First clear .bss */
    if (bss)
        __clear_bss((const struct KernelBSS *)bss->ti_Data);

    /*
     * ... then switch to initial stack and jump to target address.
     * We set rbp to 0 and use call here in order to get correct stack traces
     * if the boot task crashes. Otherwise backtrace goes beyond this location
     * into memory areas with undefined contents.
     */
    asm volatile("movq %1, %%rsp\n\t"
                 "movq $0, %%rbp\n\t"
                 "call *%2\n"::"D"(msg), "r"(boot_stack + STACK_SIZE), "r"(target));
}

/*
 * This is the main entry point.
 * We run from here both at first boot and upon reboot.
 */
void kernel_cstart(const struct TagItem *start_msg)
{
    struct MinList memList;
    struct TagItem *msg = (struct TagItem *)start_msg;
    struct MemHeader *mh, *mh2;
    struct mb_mmap *mmap = NULL;
    IPTR mmap_len = 0, addr = 0, klo  = 0, memtop = 0;
    struct TagItem *tag;
#if defined(__AROSEXEC_SMP__)
    struct X86SchedulerPrivate  *scheduleData;
#endif
    UWORD *ranges[] =
    {
        NULL,
        NULL,
        (UWORD *)-1
    };

    /* Enable fxsave/fxrstor */
    wrcr(cr4, rdcr(cr4) | _CR4_OSFXSR | _CR4_OSXMMEXCPT);

    D(bug("[Kernel] %s: Boot data: 0x%p\n", __func__, __KernBootPrivate));
    DSTACK(bug("[Kernel] %s: Boot stack: 0x%p - 0x%p\n", __func__, boot_stack, boot_stack + STACK_SIZE));

    if (__KernBootPrivate == NULL)
    {
        /* This is our first start. */
        struct vbe_mode *vmode = NULL;
        char *cmdline = NULL;
        IPTR khi;

        /* We need highest KS address and memory map to begin the work */
        khi      = LibGetTagData(KRN_KernelHighest, 0, msg);
        mmap     = (struct mb_mmap *)LibGetTagData(KRN_MMAPAddress, 0, msg);
        mmap_len = LibGetTagData(KRN_MMAPLength, 0, msg);

        if ((!khi) || (!mmap) || (!mmap_len))
        {
            krnPanic(NULL, "Incomplete information from the bootstrap\n"
                           "\n"
                           "Kickstart top: 0x%p\n"
                           "Memory map: address 0x%p, length %lu\n", khi, mmap, mmap_len);
        }

        /*
         * Our boot taglist is located just somewhere in memory. Additionally, it's very fragmented
         * (its linked data, like VBE information, were also placed just somewhere, by GRUB.
         * Now we need some memory to gather these things together. This memory will be preserved
         * accross warm restarts.
         * We know the bootstrap has reserved some space right beyond the kickstart. We get our highest
         * address, and use memory map to locate topmost address of this area.
         */
        khi  = AROS_ROUNDUP2(khi + 1, sizeof(APTR));
        mmap = mmap_FindRegion(khi, mmap, mmap_len);

        if (!mmap)
        {
            krnPanic(NULL, "Inconsistent memory map or kickstart placement\n"
                           "Kickstart region not found");
        }

        if (mmap->type != MMAP_TYPE_RAM)
        {
            krnPanic(NULL, "Inconsistent memory map or kickstart placement\n"
                           "Reserved memory overwritten\n"
                           "Region 0x%p - 0x%p type %d\n"
                           "Kickstart top 0x%p", mmap->addr, mmap->addr + mmap->len - 1, mmap->type, khi);
        }

        /* Initialize boot-time memory allocator */
        BootMemPtr   = (void *)khi;
        BootMemLimit = (void *)mmap->addr + mmap->len;

        D(bug("[Kernel] Bootinfo storage 0x%p - 0x%p\n", BootMemPtr, BootMemLimit));

        /*
         * Our boot taglist is placed by the bootstrap just somewhere in memory.
         * The first thing is to move it into some safe place.
         */

        /* This will relocate the taglist itself */
        RelocateBootMsg(msg);

        /*
         * Now relocate linked data.
         * Here we actually process only tags we know about and expect to get.
         * For example, we are not going to receive KRN_HostInterface or KRN_OpenfirmwareTree.
         */
        msg = BootMsg;
        while ((tag = LibNextTagItem(&msg)))
        {
            switch (tag->ti_Tag)
            {
            case KRN_KernelBss:
                RelocateBSSData(tag);
                break;

            case KRN_MMAPAddress:
                RelocateTagData(tag, mmap_len);
                break;

            case KRN_VBEModeInfo:
                RelocateTagData(tag, sizeof(struct vbe_mode));
                vmode = (struct vbe_mode *)tag->ti_Data;
                break;

            case KRN_VBEControllerInfo:
                RelocateTagData(tag, sizeof(struct vbe_controller));
                break;

            case KRN_CmdLine:
                RelocateStringData(tag);
                cmdline = (char *)tag->ti_Data;
                break;

            case KRN_BootLoader:
                RelocateStringData(tag);
                break;
            }
        }

        /* Now allocate KernBootPrivate */
        __KernBootPrivate = krnAllocBootMem(sizeof(struct KernBootPrivate));

        if (cmdline && vmode && vmode->phys_base && strstr(cmdline, "vesahack"))
        {
            bug("[Kernel] VESA debugging hack activated\n");

            /*
             * VESA hack.
             * It divides screen height by 2 and increments framebuffer pointer.
             * This allows VESA driver to use only upper half of the screen, while
             * lower half will still be used for debug output.
             */
            vmode->y_resolution >>= 1;

            __KernBootPrivate->debug_y_resolution = vmode->y_resolution;
            __KernBootPrivate->debug_framebuffer  = (void *)(unsigned long)vmode->phys_base + vmode->y_resolution * vmode->bytes_per_scanline;
        }

        if (cmdline && strstr(cmdline, "notlsf"))
            allocator = ALLOCATOR_STD;
    }

    /* We are x86-64, and we know we always have APIC. */
    __KernBootPrivate->_APICBase = core_APIC_GetBase();

    /* Pre-Allocare TLS & GDT */
    if (!__KernBootPrivate->BOOTTLS)
        __KernBootPrivate->BOOTTLS = core_AllocBootTLS(__KernBootPrivate);
    if (!__KernBootPrivate->BOOTGDT)
        __KernBootPrivate->BOOTGDT = core_AllocBootGDT(__KernBootPrivate);
    if (!__KernBootPrivate->TSS)
        __KernBootPrivate->TSS = core_AllocBootTSS(__KernBootPrivate);

    /* Setup GDT */
    core_SetupGDT(__KernBootPrivate, 0,
        __KernBootPrivate->BOOTGDT,
        __KernBootPrivate->BOOTTLS,
        __KernBootPrivate->TSS);

    if (!__KernBootPrivate->SystemStack)
    {
        /*
         * Allocate our supervisor stack from boot-time memory.
         * It will be protected from user's intervention.
         * Allocate actually three stacks: panic, supervisor, ring1.
         * Note that we do the actual allocation only once. The region is kept
         * in __KernBootPrivate which survives warm reboots.
         */
        __KernBootPrivate->SystemStack = (IPTR)krnAllocBootMem(STACK_SIZE * 3);

        DSTACK(bug("[Kernel] %s: Allocated supervisor stack 0x%p - 0x%p\n",
                    __func__,
                    __KernBootPrivate->SystemStack,
                    __KernBootPrivate->SystemStack + STACK_SIZE * 3));
    }

    D(
        bug("[Kernel] %s: launching on BSP APIC ID %03u\n", __func__, core_APIC_GetID(__KernBootPrivate->_APICBase));
        bug("[Kernel] %s:                apicbase : 0x%p\n", __func__, __KernBootPrivate->_APICBase);
        bug("[Kernel] %s:                GDT      : 0x%p\n", __func__, __KernBootPrivate->BOOTGDT);
        bug("[Kernel] %s:                TLS      : 0x%p\n", __func__, __KernBootPrivate->BOOTTLS);
        bug("[Kernel] %s:                TSS      : 0x%p\n", __func__, __KernBootPrivate->TSS);
    )

    /* Load the TSS, and GDT */
    core_CPUSetup(0, __KernBootPrivate->BOOTGDT, __KernBootPrivate->SystemStack);

    D(bug("[Kernel] %s: preparing interrupt vectors\n", __func__));
    /* Set-up the IDT */
    __KernBootPrivate->BOOTIDT = core_AllocBootIDT(__KernBootPrivate);
    D(bug("[Kernel] %s:                IDT      : 0x%p\n", __func__, __KernBootPrivate->BOOTIDT);)
    core_SetupIDT(0, (apicidt_t *)__KernBootPrivate->BOOTIDT);

    /* Set-up MMU */
    // Re-read mmap pointer, since we have modified it previously...
    mmap = (struct mb_mmap *)LibGetTagData(KRN_MMAPAddress, 0, BootMsg);
    memtop = mmap_LargestAddress(mmap, mmap_len);
    D(bug("[Kernel] %s: memtop @ 0x%p\n", __func__, memtop));
    core_SetupMMU(&__KernBootPrivate->MMU, memtop);

    /*
     * Here we ended all boot-time allocations.
     * We won't do them again, for example on warm reboot. All our areas are stored in struct KernBootPrivate.
     * We are going to make this area read-only and reset-proof.
     */
    if (!kick_highest)
    {
        D(bug("[Kernel] Boot-time setup complete\n"));
        kick_highest = AROS_ROUNDUP2((IPTR)BootMemPtr, PAGE_SIZE);
    }

    D(bug("[Kernel] End of kickstart area 0x%p\n", kick_highest));

    /*
     * Obtain the needed data from the boot taglist.
     * We need to do this even on first boot, because the taglist and its data
     * have been moved to the permanent storage.
     */
    msg = BootMsg;
    while ((tag = LibNextTagItem(&msg)))
    {
        switch (tag->ti_Tag)
        {
        case KRN_KernelBase:
            /*
             * KRN_KernelBase is actually a border between read-only
             * (code) and read-write (data) sections of the kickstart.
             * read-write section goes to lower addresses from this one,
             * so we align it upwards in order not to make part of RW data
             * read-only.
             */
            addr = AROS_ROUNDUP2(tag->ti_Data, PAGE_SIZE);
            break;

        case KRN_KernelLowest:
            klo = AROS_ROUNDDOWN2(tag->ti_Data, PAGE_SIZE);
            break;

        case KRN_MMAPAddress:
            mmap = (struct mb_mmap *)tag->ti_Data;
            break;
            
        case KRN_MMAPLength:
            mmap_len = tag->ti_Data;
            break;
        }
    }

    /* Sanity check */
    if ((!klo) || (!addr))
    {
        krnPanic(NULL, "Incomplete information from the bootstrap\n"
                       "\n"
                       "Kickstart lowest 0x%p, base 0x%p\n", klo, addr);
    }

    /*
     * Explore memory map and create MemHeaders.
     * We reserve one page (PAGE_SIZE) at zero address. We will protect it.
     */
    NEWLIST(&memList);
    mmap_InitMemory(mmap, mmap_len, &memList, klo, kick_highest, PAGE_SIZE, PC_Memory, allocator);

    D(bug("[Kernel] kernel_cstart: Booting exec.library...\n"));

    /*
     * mmap_InitMemory() adds MemHeaders to the list in the order they were created.
     * I. e. highest addresses are added last.
     * Take highest region in order to create SysBase in it.
     */
    mh = (struct MemHeader *)REMTAIL(&memList);
    D(bug("[Kernel] Initial MemHeader: 0x%p - 0x%p (%s)\n", mh->mh_Lower, mh->mh_Upper, mh->mh_Node.ln_Name));

    if (SysBase)
    {
        D(bug("[Kernel] Got old SysBase 0x%p...\n", SysBase));
        /*
         * Validate existing SysBase pointer.
         * Here we check that if refers to a valid existing memory region.
         * Checksums etc are checked in arch-independent code in exec.library.
         * It's enough to use only size of public part. Anyway, SysBase will be
         * reallocated by PrepareExecBase(), it will just keep over some data from
         * public part (KickMemPtr, KickTagPtr and capture vectors).
         */
        if (!mmap_ValidateRegion((unsigned long)SysBase, sizeof(struct ExecBase), mmap, mmap_len))
        {
            D(bug("[Kernel] ... invalidated\n"));
            SysBase = NULL;
        }
    }

    /* This handles failures itself */
    ranges[0] = (UWORD *)klo;
    ranges[1] = (UWORD *)kick_highest;
    krnPrepareExecBase(ranges, mh, BootMsg);

    krnCreateROMHeader("Kickstart ROM", (APTR)klo, (APTR)kick_highest);

#if defined(__AROSEXEC_SMP__)
    D(bug("[Kernel] Allocating CPU #0 Scheduling Data\n"));
    scheduleData = AllocMem(sizeof(struct X86SchedulerPrivate), MEMF_PUBLIC|MEMF_CLEAR);
    if (!scheduleData)
        krnPanic(KernelBase, "Failed to Allocate Boot Processor Scheduling Data!");

    core_InitScheduleData(scheduleData);

    TLS_SET(ScheduleData, scheduleData);
    D(bug("[Kernel] Scheduling Data @ 0x%p\n", TLS_GET(ScheduleData)));
#endif
    /*
     * Now we have working exec.library memory allocator.
     * Move console mirror buffer away from unused memory.
     * WARNING!!! Do not report anything in the debug log before this is done. Remember that sequental
     * AllocMem()s return sequental blocks! And right beyond our allocated area there will be MemChunk.
     * Between krnPrepareExecBase() and this AllocMem() upon warm reboot console mirror buffer is set
     * to an old value right above ExecBase. During krnPrepareExecBase() a MemChunk is built there,
     * which can be overwritten by bootconsole, especially if the output scrolls.
     */
    if (scr_Type == SCR_GFX)
    {
        char *mirror = AllocMem(scr_Width * scr_Height, MEMF_PUBLIC);

        fb_SetMirror(mirror);
    }

    D(bug("[Kernel] Created SysBase at 0x%p (pointer at 0x%p), MemHeader 0x%p\n", SysBase, &SysBase, mh));

    /* Block all user's access to zero page */
    core_ProtKernelArea(0, PAGE_SIZE, 1, 0, 0);

    /* Store important private data */
    TLS_SET(SysBase, SysBase);

    /* Provide information about our supevisor stack. Useful at least for diagnostics. */
    SysBase->SysStkLower = (APTR)__KernBootPrivate->SystemStack;
    SysBase->SysStkUpper = (APTR)__KernBootPrivate->SystemStack + STACK_SIZE * 3;

    /*
     * Make kickstart code area read-only.
     * We do it only after ExecBase creation because SysBase pointer is put
     * into .rodata. This way we prevent it from ocassional modification by buggy software.
     */
    core_ProtKernelArea(addr, kick_highest - addr, 1, 0, 1);

    /* Transfer the rest of memory list into SysBase */
    D(bug("[Kernel] Transferring memory list into SysBase...\n"));
    for (mh = (struct MemHeader *)memList.mlh_Head; mh->mh_Node.ln_Succ; mh = mh2)
    {
        mh2 = (struct MemHeader *)mh->mh_Node.ln_Succ;

        D(bug("[Kernel] * 0x%p - 0x%p (%s pri %d)\n", mh->mh_Lower, mh->mh_Upper, mh->mh_Node.ln_Name, mh->mh_Node.ln_Pri));
        Enqueue(&SysBase->MemList, &mh->mh_Node);
    }

    /*
     * RTF_SINGLETASK residents are called with supervisor privilege level.
     * Original AmigaOS(tm) does the same, some Amiga hardware expansion ROM
     * rely on it. Here we continue the tradition, because it's useful for
     * acpica.library (which needs to look for RSDP in the first 1M)
     */
    InitCode(RTF_SINGLETASK, 0);

    /*
     * After InitCode(RTF_SINGLETASK) we may have acpica.library
     * To perform some basic initialization.
     */
    PlatformPostInit();

    /* Drop privileges down to user mode before calling RTF_COLDSTART */
    D(bug("[Kernel] Leaving supervisor mode\n"));
    krnLeaveSupervisorRing(FLAGS_INTENABLED);

    /*
     * We are fully done. Run exec.library and the rest.
     * exec.library will be the first resident to run. It will enable interrupts and multitasking for us.
     */
    InitCode(RTF_COLDSTART, 0);

    /* The above must not return */
    krnPanic(KernelBase, "System Boot Failed!");
}

/* Small delay routine used by exec_cinit initializer */
asm("\ndelay:\t.short   0x00eb\n\tretq");

/* Our boot-time stack */
static char boot_stack[STACK_SIZE] __attribute__((aligned(16)));

void core_SetupGDT
(
    struct KernBootPrivate *__KernBootPrivate,
    apicid_t cpuNo,
    APTR gdtBase,
    APTR gdtTLS,
    APTR gdtTSS
)
{
    struct gdt_64bit *gdtPtr = (struct gdt_64bit *)gdtBase;
    struct tss_64bit *tssPtr = (struct tss_64bit *)gdtTSS;
    int i;

    D(bug("[Kernel] %s(%03u, 0x%p, 0x%p, 0x%p)\n", __func__, cpuNo, gdtBase, gdtTLS, gdtTSS));

    // TODO: ASSERT GDT is aligned
    
    /* Supervisor segments */
    gdtPtr->super_cs.type               = 0x1a;                                         /* code segment */
    gdtPtr->super_cs.dpl                = 0;                                            /* supervisor level */
    gdtPtr->super_cs.p                  = 1;                                            /* present */
    gdtPtr->super_cs.l                  = 1;                                            /* long (64-bit) one */
    gdtPtr->super_cs.d                  = 0;                                            /* must be zero */
    gdtPtr->super_cs.limit_low          = 0xffff;
    gdtPtr->super_cs.limit_high         = 0xf;
    gdtPtr->super_cs.g                  = 1;

    gdtPtr->super_ds.type               = 0x12;                                         /* data segment */
    gdtPtr->super_ds.dpl                = 0;                                            /* supervisor level */
    gdtPtr->super_ds.p                  = 1;                                            /* present */
    gdtPtr->super_ds.l                  = 1;                                            /* long (64-bit) one */
    gdtPtr->super_ds.d                  = 1;                                            /*  */
    gdtPtr->super_ds.limit_low          = 0xffff;
    gdtPtr->super_ds.limit_high         = 0xf;
    gdtPtr->super_ds.g                  = 1;

    /* User mode segments */
    gdtPtr->user_cs.type                = 0x1a;                                         /* code segment */
    gdtPtr->user_cs.dpl                 = 3;                                            /* User level */
    gdtPtr->user_cs.p                   = 1;                                            /* present */
    gdtPtr->user_cs.l                   = 1;                                            /* long mode */
    gdtPtr->user_cs.d                   = 0;                                            /* must be zero */
    gdtPtr->user_cs.limit_low           = 0xffff;
    gdtPtr->user_cs.limit_high          = 0xf;
    gdtPtr->user_cs.g                   = 1;

    gdtPtr->user_cs32.type              = 0x1a;    /* code segment for legacy 32-bit code. NOT USED YET! */
    gdtPtr->user_cs32.dpl               = 3;                                            /* user level */
    gdtPtr->user_cs32.p                 = 1;                                            /* present */
    gdtPtr->user_cs32.l                 = 0;                                            /* 32-bit mode */
    gdtPtr->user_cs32.d                 = 1;                                            /* 32-bit code */
    gdtPtr->user_cs32.limit_low         = 0xffff;
    gdtPtr->user_cs32.limit_high        = 0xf;
    gdtPtr->user_cs32.g                 = 1;

    gdtPtr->user_ds.type                = 0x12;                                         /* data segment */
    gdtPtr->user_ds.dpl                 = 3;                                            /* user level */
    gdtPtr->user_ds.p                   = 1;                                            /* present */
    gdtPtr->user_ds.l                   = 1;                                            /* long mode */
    gdtPtr->user_ds.d                   = 1;
    gdtPtr->user_ds.limit_low           = 0xffff;
    gdtPtr->user_ds.limit_high          = 0xf;
    gdtPtr->user_ds.g                   = 1;

    for (i=0; i < 16; i++)
    {
        const unsigned long tss_limit = sizeof(struct tss_64bit) * 16 - 1;

        /* Task State Segment */
        gdtPtr->tss[i].tss_low.type       = 0x09;                                       /* 64-bit TSS */
        gdtPtr->tss[i].tss_low.dpl        = 3;                                          /* User mode task */
        gdtPtr->tss[i].tss_low.p          = 1;                                          /* present */
        gdtPtr->tss[i].tss_low.l          = 1;                                            /* long mode */
        gdtPtr->tss[i].tss_low.d          = 1;
        gdtPtr->tss[i].tss_low.limit_low  = tss_limit;
        gdtPtr->tss[i].tss_low.base_low   = ((unsigned long)&tssPtr[i]) & 0xffff;
        gdtPtr->tss[i].tss_low.base_mid   = (((unsigned long)&tssPtr[i]) >> 16) & 0xff;
        gdtPtr->tss[i].tss_low.limit_high = (tss_limit >> 16) & 0x0f;
        gdtPtr->tss[i].tss_low.base_high  = (((unsigned long)&tssPtr[i]) >> 24) & 0xff;
        gdtPtr->tss[i].tss_high.base_ext  = 0;                                          /* is within 4GB :-D */
    }

    gdtPtr->gs.type                     = 0x12;                                         /* data segment */
    gdtPtr->gs.dpl                      = 3;                                            /* user level */
    gdtPtr->gs.p                        = 1;                                            /* present */
    gdtPtr->gs.l                        = 1;                                            /* long mode */
    gdtPtr->gs.d                        = 1;
    gdtPtr->gs.base_low                 = (intptr_t)gdtTLS & 0xffff;
    gdtPtr->gs.base_mid                 = ((intptr_t)gdtTLS >> 16) & 0xff;
    gdtPtr->gs.base_high                = ((intptr_t)gdtTLS >> 24) & 0xff;
    gdtPtr->gs.g                        = 1;
}
 
void core_CPUSetup(apicid_t cpuNo, APTR cpuGDT, IPTR SystemStack)
{
    struct segment_selector cpuGDTsel;
    struct tss_64bit *tssBase = __KernBootPrivate->TSS;

    D(bug("[Kernel] %s(%03u, 0x%p, 0x%p)\n", __func__, cpuNo, cpuGDT, SystemStack));

    /*
     * At the moment two of three stacks are reserved. IST is not used (indexes == 0 in interrupt gates)
     * and ring 1 is not used either. However, the space pointed to by IST is used as a temporary stack
     * for warm restart routine.
     */
    tssBase[cpuNo].ist1 = SystemStack + STACK_SIZE     - 16;    /* Interrupt stack entry 1 (failsafe)    */
    tssBase[cpuNo].rsp0 = SystemStack + STACK_SIZE * 2 - 16;    /* Ring 0 (Supervisor)                  */
    tssBase[cpuNo].rsp1 = SystemStack + STACK_SIZE * 3 - 16;    /* Ring 1 (reserved)                    */
    
    D(bug("[Kernel] %s[%03u]: Reloading -:\n", __func__, cpuNo));
    D(bug("[Kernel] %s[%03u]:     CPU GDT @ 0x%p\n", __func__, cpuNo, cpuGDT));
    D(bug("[Kernel] %s[%03u]:     CPU TSS @ 0x%p\n", __func__, cpuNo, &tssBase[cpuNo]));

    cpuGDTsel.size = sizeof(struct gdt_64bit) - 1;
    cpuGDTsel.base = (unsigned long)cpuGDT;
    asm volatile ("lgdt %0"::"m"(cpuGDTsel));
    asm volatile ("ltr %w0"::"r"(TASK_SEG + (cpuNo << 4)));
    asm volatile ("mov %0,%%gs"::"a"(USER_GS));
}

/* Boot-Time Allocation routines ... */

static APTR core_AllocBootTLS(struct KernBootPrivate *__KernBootPrivate)
{
    tls_t *tlsPtr;

    tlsPtr = (tls_t *)krnAllocBootMem(sizeof(tls_t));

    return (APTR)tlsPtr;
}

static APTR core_AllocBootTSS(struct KernBootPrivate *__KernBootPrivate)
{
    struct tss_64bit *tssPtr;

    tssPtr = krnAllocBootMemAligned(sizeof(struct tss_64bit) * 16, 128);

    return (APTR)tssPtr;
}

static APTR core_AllocBootIDT(struct KernBootPrivate *__KernBootPrivate)
{
    if (!__KernBootPrivate->BOOTIDT)
        __KernBootPrivate->BOOTIDT = krnAllocBootMemAligned(sizeof(struct int_gate_64bit) * 256, 256);

    return (APTR)__KernBootPrivate->BOOTIDT;
}

static APTR core_AllocBootGDT(struct KernBootPrivate *__KernBootPrivate)
{
    struct gdt_64bit *gdtPtr;

    gdtPtr = (struct gdt_64bit *)krnAllocBootMemAligned(sizeof(struct gdt_64bit), 128);

    return (APTR)gdtPtr;
}
