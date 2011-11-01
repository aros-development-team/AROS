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
#include "kernel_bootmem.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "kernel_mmap.h"
#include "kernel_romtags.h"
#include "apic.h"
#include "smp.h"
#include "tls.h"

#define D(x)
#define DSTACK(x)

/* Common IBM PC memory layout */
static const struct MemRegion PC_Memory[] =
{
    /*
     * Give low memory a bit lower priority. This will help us to locate its MemHeader (the last one in the list).
     * We explicitly need low memory for SMP bootstrap.
     */
    {0x000000000, 0x000100000, "Low memory"    , -6, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_CHIP|MEMF_31BIT|MEMF_24BITDMA},
    {0x000100000, 0x001000000, "ISA DMA memory", -5, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_CHIP|MEMF_31BIT|MEMF_24BITDMA},
    /*
     * FIXME: The following two entries should also be CHIP. trackdisk.device and i386 port
     * fix is needed (use MEMF_24BITDMA instead of MEMF_CHIP for 24-bit ISA DMA-capable area.
     */
    {0x001000000, 0x080000000, "31-bit memory" ,  0, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_FAST|MEMF_31BIT		},
    /*
     * FIXME: Our MMU mapping supports only 4GB address space.
     * We can't enable more right now because lots of RAM would be required for MMU tables,
     * and it will be irrational to reserve so large boot-time region (AROS will fail to boot
     * up on systems with relatively small amount of RAM).
     * MMU structures need to be allocated dynamically from a working memory. Waiting for Michal's
     * page allocator to implement this...
    {0x080000000, -1         , "Upper memory"  , 10, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_FAST                         }, */
    {0          , 0          , NULL            ,  0, 0                                                                  }
};

/*
 * Boot-time global variables.
 * __KernBootPrivate needs to survive accross warm reboots, so it's put into .data.
 * SysBase is intentionally put into .rodata. This way we prevent it from being modified.
 */
__attribute__((section(".data"))) struct KernBootPrivate *__KernBootPrivate = NULL;
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

    bug("AROS64 - The AROS Research OS, 64-bit version. Compiled %s\n", __DATE__);
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

/* Print panic string and halt */
static void panic(void)
{
    bug("*** SYSTEM PANIC ***\n");
    while (1);
}

/*
 * This is the main entry point.
 * We run from here both at first boot and upon reboot.
 */
void kernel_cstart(const struct TagItem *msg)
{
    struct MinList memList;
    struct MemHeader *mh, *mh2;
    struct mb_mmap *mmap = NULL;
    IPTR mmap_len = 0;
    IPTR addr = 0;
    IPTR klo  = 0;
    IPTR khi;
    struct TagItem *tag;
    UBYTE _APICID;
    UWORD *ranges[] = {NULL, NULL, (UWORD *)-1};

    /* Enable fxsave/fxrstor */ 
    wrcr(cr4, rdcr(cr4) | _CR4_OSFXSR | _CR4_OSXMMEXCPT);

    D(bug("[Kernel] Boot data: 0x%p\n", __KernBootPrivate));
    DSTACK(bug("[Kernel] Boot stack: 0x%p - 0x%p\n", boot_stack, boot_stack + STACK_SIZE));
    if (__KernBootPrivate == NULL)
    {
    	/* This is our first start. */
        struct vbe_mode *vmode = NULL;
        char *cmdline = NULL;

	tag = LibFindTagItem(KRN_KernelHighest, msg);
        if (!tag)
        {
            bug("Incomplete information from the bootstrap\n");
            bug("Highest kickstart address is not supplied\n");

            panic();
        }

	/*
	 * Initialize boot-time memory allocator.
	 * We know the bootstrap has reserved some space right beyond the kickstart.
	 */
        BootMemPtr = (void *)AROS_ROUNDUP2(tag->ti_Data + 1, sizeof(APTR));

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
	mmap_len = LibGetTagData(KRN_MMAPLength, 0, BootMsg);
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
    }

    D(bug("[Kernel] End of kickstart data area: 0x%p\n", BootMemPtr));

    /* Prepare GDT */
    core_SetupGDT(__KernBootPrivate);

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

    	DSTACK(bug("[Kernel] Allocated supervisor stack 0x%p - 0x%p\n",
    		   __KernBootPrivate->SystemStack, __KernBootPrivate->SystemStack + STACK_SIZE * 3));
    }

    /* We are x86-64, and we know we always have APIC. */
    __KernBootPrivate->_APICBase = core_APIC_GetBase();
    _APICID = core_APIC_GetID(__KernBootPrivate->_APICBase);
    D(bug("[Kernel] kernel_cstart: launching on BSP APIC ID %d, base @ %p\n", _APICID, __KernBootPrivate->_APICBase));

    /* Set TSS, GDT, LDT and MMU up */
    core_CPUSetup(_APICID, __KernBootPrivate->SystemStack);
    core_SetupIDT(__KernBootPrivate);
    core_SetupMMU(__KernBootPrivate);

    /*
     * Here we ended all boot-time allocations.
     * We won't do them again, for example on warm reboot. All our areas are stored in struct KernBootPrivate.
     * We are going to make this area read-only and reset-proof.
     */
    khi = AROS_ROUNDUP2((IPTR)BootMemPtr, PAGE_SIZE);
    D(bug("[Kernel] Boot-time setup complete, end of kickstart area 0x%p\n", khi));

    /* Obtain the needed data from the boot taglist */
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
    if ((!klo) || (!addr) || (!mmap) || (!mmap_len))
    {
        bug("Incomplete information from the bootstrap\n");
        bug("Kickstart addresses: lowest  0x%p, base   0x%p", klo, addr);
        bug("Memory map         : address 0x%p, length %lu\n", mmap, mmap_len);

        panic();
    }

    /*
     * Explore memory map and create MemHeaders.
     * We reserve one page (PAGE_SIZE) at zero address. We will protect it.
     */
    NEWLIST(&memList);
    mmap_InitMemory(mmap, mmap_len, &memList, klo, khi, PAGE_SIZE, PC_Memory);

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

    ranges[0] = (UWORD *)klo;
    ranges[1] = (UWORD *)khi;
    if (!krnPrepareExecBase(ranges, mh, BootMsg))
    {
        bug("Failed to create exec.library base\n");

        panic();
    }

    D(bug("[Kernel] Created SysBase at 0x%p (pointer at 0x%p), MemHeader 0x%p\n", SysBase, &SysBase, mh));

    /* Block all user's access to zero page */
    core_ProtKernelArea(0, PAGE_SIZE, 1, 0, 0);

    /*
     * Now we have working exec.library memory allocator.
     * Move console mirror buffer away from unused memory.
     */
    if (scr_Type == SCR_GFX)
    {
	char *mirror = AllocMem(scr_Width * scr_Height, MEMF_PUBLIC);

	fb_SetMirror(mirror);
    }

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
    core_ProtKernelArea(addr, khi - addr, 1, 0, 1);

    /* Transfer the rest of memory list into SysBase */
    D(bug("[Kernel] Transferring memory list into SysBase...\n"));
    for (mh = (struct MemHeader *)memList.mlh_Head; mh->mh_Node.ln_Succ; mh = mh2)
    {
        mh2 = (struct MemHeader *)mh->mh_Node.ln_Succ;

	D(bug("[Kernel] * 0x%p - 0x%p (%s)\n", mh->mh_Lower, mh->mh_Upper, mh->mh_Node.ln_Name));
	Enqueue(&SysBase->MemList, &mh->mh_Node);
    }

    /*
     * RTF_SINGLETASK residents are called with supervisor privilege level.
     * Original AmigaOS(tm) does the same, some Amiga hardware expansion ROM
     * rely on it. Here we continue the tradition, because it's useful for
     * acpi.resource (which needs to look for RSDP in zero page).
     */
    InitCode(RTF_SINGLETASK, 0);

    /*
     * After InitCode(RTF_SINGLETASK) we may have acpi.resource.
     * Now we can use ACPI information in order to set up advanced things (SMP, APIC, etc).
     * Interrupts are still disabled and we are still supervisor.
     */
    acpi_Initialize();

    /* Now initialize our interrupt controller (XT-PIC or APIC) */
    ictl_Initialize();

    /* The last thing to do is to start up secondary CPU cores (if any) */
    smp_Initialize();

    /* Drop privileges down to user mode before calling RTF_COLDSTART */
    D(bug("[Kernel] Leaving supervisor mode\n"));
    asm volatile (
            "mov %[user_ds],%%ds\n\t"   // Load DS and ES
            "mov %[user_ds],%%es\n\t"
            "mov %%rsp,%%r12\n\t"
            "pushq %[ds]\n\t"      	// SS
            "pushq %%r12\n\t"           // rSP
            "pushq $0x3002\n\t"         // rFLAGS
            "pushq %[cs]\n\t"		// CS
            "pushq $1f\n\t"
            "iretq\n 1:"
            ::[user_ds]"r"(USER_DS),[ds]"i"(USER_DS),[cs]"i"(USER_CS):"r12");

    D(bug("[Kernel] Done?! Still here?\n"));

    /*
     * We are fully done. Run exec.library and the rest.
     * exec.library will be the first resident to run. It will enable interrupts and multitasking for us.
     */
    InitCode(RTF_COLDSTART, 0);

    bug("[Kernel] ERROR: System Boot Failed!\n");
    panic();
}

/* Small delay routine used by exec_cinit initializer */
asm("\ndelay:\t.short   0x00eb\n\tretq");

/* Our boot-time stack */
static char boot_stack[STACK_SIZE] __attribute__((aligned(16)));

struct gdt_64bit
{
    struct segment_desc seg0;      /* seg 0x00 */
    struct segment_desc super_cs;  /* seg 0x08 */
    struct segment_desc super_ds;  /* seg 0x10 */
    struct segment_desc user_cs32; /* seg 0x18 */
    struct segment_desc user_ds;   /* seg 0x20 */
    struct segment_desc user_cs;   /* seg 0x28 */
    struct segment_desc gs;        /* seg 0x30 */
    struct segment_desc ldt;       /* seg 0x38 */
    struct
    {
        struct segment_desc tss_low;   /* seg 0x40... */
        struct segment_ext  tss_high;
    } tss[16];
};

void core_SetupGDT(struct KernBootPrivate *__KernBootPrivate)
{
    struct gdt_64bit *GDT;
    struct tss_64bit *TSS;
    intptr_t tls_ptr;
    int i;

    D(bug("[Kernel] core_SetupGDT(0x%p)\n", __KernBootPrivate));

    if (!__KernBootPrivate->GDT)
    {
        __KernBootPrivate->system_tls = krnAllocBootMem(sizeof(tls_t));
        __KernBootPrivate->GDT        = krnAllocBootMemAligned(sizeof(struct gdt_64bit), 128);
        __KernBootPrivate->TSS        = krnAllocBootMemAligned(sizeof(struct tss_64bit) * 16, 128);

        D(bug("[Kernel] Allocated GDT 0x%p, TLS 0x%p\n", __KernBootPrivate->GDT, __KernBootPrivate->system_tls));
    }

    GDT = __KernBootPrivate->GDT;
    TSS = __KernBootPrivate->TSS;

    /* Supervisor segments */
    GDT->super_cs.type=0x1a;     /* code segment */
    GDT->super_cs.dpl=0;         /* supervisor level */
    GDT->super_cs.p=1;           /* present */
    GDT->super_cs.l=1;           /* long (64-bit) one */
    GDT->super_cs.d=0;           /* must be zero */
    GDT->super_cs.limit_low=0xffff;
    GDT->super_cs.limit_high=0xf;
    GDT->super_cs.g=1;

    GDT->super_ds.type=0x12;     /* data segment */
    GDT->super_ds.dpl=0;         /* supervisor level */
    GDT->super_ds.p=1;           /* present */
    GDT->super_ds.limit_low=0xffff;
    GDT->super_ds.limit_high=0xf;
    GDT->super_ds.g=1;
    GDT->super_ds.d=1;

    /* User mode segments */
    GDT->user_cs.type=0x1a;      /* code segment */
    GDT->user_cs.dpl=3;          /* User level */
    GDT->user_cs.p=1;            /* present */
    GDT->user_cs.l=1;            /* long mode */
    GDT->user_cs.d=0;            /* must be zero */
    GDT->user_cs.limit_low=0xffff;
    GDT->user_cs.limit_high=0xf;
    GDT->user_cs.g=1;

    GDT->user_cs32.type=0x1a;    /* code segment for legacy 32-bit code. NOT USED YET! */
    GDT->user_cs32.dpl=3;        /* user level */
    GDT->user_cs32.p=1;          /* present */
    GDT->user_cs32.l=0;          /* 32-bit mode */
    GDT->user_cs32.d=1;          /* 32-bit code */
    GDT->user_cs32.limit_low=0xffff;
    GDT->user_cs32.limit_high=0xf;
    GDT->user_cs32.g=1;

    GDT->user_ds.type=0x12;      /* data segment */
    GDT->user_ds.dpl=3;    	     /* user level */
    GDT->user_ds.p=1;            /* present */
    GDT->user_ds.limit_low=0xffff;
    GDT->user_ds.limit_high=0xf;
    GDT->user_ds.g=1;
    GDT->user_ds.d=1;

    for (i=0; i < 16; i++)
    {
        const unsigned long tss_limit = sizeof(struct tss_64bit) * 16 - 1;

        /* Task State Segment */
        GDT->tss[i].tss_low.type       = 0x09;				      		/* 64-bit TSS */
        GDT->tss[i].tss_low.limit_low  = tss_limit;
        GDT->tss[i].tss_low.base_low   = ((unsigned long)&TSS[i]) & 0xffff;
        GDT->tss[i].tss_low.base_mid   = (((unsigned long)&TSS[i]) >> 16) & 0xff;
        GDT->tss[i].tss_low.dpl        = 3;						/* User mode task */
        GDT->tss[i].tss_low.p          = 1;						/* present */
        GDT->tss[i].tss_low.limit_high = (tss_limit >> 16) & 0x0f;
        GDT->tss[i].tss_low.base_high  = (((unsigned long)&TSS[i]) >> 24) & 0xff;
        GDT->tss[i].tss_high.base_ext  = 0;						/* is within 4GB :-D */
    }

    tls_ptr = (intptr_t)__KernBootPrivate->system_tls;

    GDT->gs.type=0x12;      	/* data segment */
    GDT->gs.dpl=3;    		/* user level */
    GDT->gs.p=1;            	/* present */
    GDT->gs.base_low  = tls_ptr & 0xffff;
    GDT->gs.base_mid  = (tls_ptr >> 16) & 0xff;
    GDT->gs.base_high = (tls_ptr >> 24) & 0xff;   
    GDT->gs.g=1;
    GDT->gs.d=1;
}
 
void core_CPUSetup(UBYTE _APICID, IPTR SystemStack)
{
    struct segment_selector GDT_sel;
    struct tss_64bit *TSS = __KernBootPrivate->TSS;

    D(bug("[Kernel] core_CPUSetup(%d, 0x%p)\n", _APICID, SystemStack));

    /*
     * At the moment two of three stacks are reserved. IST is not used (indexes == 0 in interrupt gates)
     * and ring 1 is not used either. However, the space pointed to by IST is used as a temporary stack
     * for warm restart routine.
     */

    TSS[_APICID].ist1 = SystemStack + STACK_SIZE     - 16;	/* Interrupt stack entry 1 (failsafe)	 */
    TSS[_APICID].rsp0 = SystemStack + STACK_SIZE * 2 - 16;	/* Ring 0 (Supervisor)		 	*/
    TSS[_APICID].rsp1 = SystemStack + STACK_SIZE * 3 - 16;	/* Ring 1 (reserved)		 	*/

    D(bug("[Kernel] core_CPUSetup[%d]: Reloading the GDT and Task Register\n", _APICID));

    GDT_sel.size = sizeof(struct gdt_64bit) - 1;
    GDT_sel.base = (uint64_t)__KernBootPrivate->GDT;
    asm volatile ("lgdt %0"::"m"(GDT_sel));
    asm volatile ("ltr %w0"::"r"(TASK_SEG + (_APICID << 4)));
    asm volatile ("mov %0,%%gs"::"a"(USER_GS));
}
