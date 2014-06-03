/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: i386-pc kernel startup code
    Lang: english
*/

#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <asm/cpu.h>
#include <exec/resident.h>
#include <proto/arossupport.h>
#include <proto/exec.h>

#include <bootconsole.h>
#include <string.h>

#include "boot_utils.h"
#include "kernel_base.h"
#include "kernel_bootmem.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "kernel_mmap.h"
#include "kernel_romtags.h"

#define D(x) x

static char boot_stack[];

static void kernel_boot(const struct TagItem *msg);
void core_Kick(struct TagItem *msg, void *target);
void kernel_cstart(const struct TagItem *msg);

/*
 * Here the history starts. We are already in flat, 32bit mode. All protections
 * are off, CPU is working in Supervisor level (CPL0). Interrupts should
 * be disabled.
 *
 * Here we run on a stack provided by the bootstrap. We can perform calls, but we
 * don't know where it is placed, so we need to switch away from it ASAP.
 */
IPTR __startup kernel_entry(struct TagItem *bootMsg, ULONG magic)
{
    if (magic == AROS_BOOT_MAGIC)
	core_Kick(bootMsg, kernel_boot);

    return -1;
}

/*
 * The real entry point for initial boot.
 * Here we initialize debug console and say "hello".
 * Warm restart skips this since the screen was taken over by display driver.
 */
static void kernel_boot(const struct TagItem *msg)
{
    /*
     * Initial framebuffer mirror will be located by default at (1MB + 4KB).
     * This is done because our bootstrap begins at 1MB, and its .tables
     * sections are placed in the beginning. We must not ocassionally overwrite
     * these sections for now because they contain boot-time data for us
     * (taglist etc).
     * A well-behaved bootstrap should give us ProtAreaEnd.
     */
    fb_Mirror = (void *)LibGetTagData(KRN_ProtAreaEnd, 0x101000, msg);
    con_InitTagList(msg);

    bug("AROS - The AROS Research OS. Compiled %s\n",__DATE__);
 
    kernel_cstart(msg);
}

/*
 * This function actually runs the kickstart from the specified address.
 * Before doing this it clears .bss sections of all modules.
 */
void core_Kick(struct TagItem *msg, void *target)
{
    const struct TagItem *bss = LibFindTagItem(KRN_KernelBss, msg);

    /* First clear .bss */
    if (bss)
    	__clear_bss((const struct KernelBSS *)bss->ti_Data);

    /*
     * ... then switch to initial stack and jump to target address.
     * We set ebp to 0 and use call here in order to get correct stack traces
     * if the boot task crashes. Otherwise backtrace goes beyond this location
     * into memory areas with undefined contents.
     */
    asm volatile("movl	%1, %%esp     \n\t"
    		 "movl	$0, %%ebp     \n\t"
		 "pushl %0            \n\t"
		 "cld                 \n\t"      /* At the startup it's very important   */
		 "cli                 \n\t"      /* to lock all interrupts. Both on the  */
		 "movb	$-1,%%al      \n\t"      /* CPU side and hardware side. We don't  */
		 "outb  %%al,$0x21    \n\t"      /* have proper structures in RAM yet.   */
		 "outb  %%al,$0xa1    \n\t"
    		 "call *%2\n"::"r"(msg), "r"(boot_stack + STACK_SIZE), "r"(target));
}

/* Common IBM PC memory layout */
static const struct MemRegion PC_Memory[] =
{
    /*
     * Give low memory a bit lower priority. This will help us to locate its MemHeader (the last one in the list).
     * We explicitly need low memory for SMP bootstrap.
     */
    {0x00000000, 0x000a0000, "Low memory"    , -6, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_CHIP|MEMF_31BIT|MEMF_24BITDMA},
    {0x00100000, 0x01000000, "ISA DMA memory", -5, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_CHIP|MEMF_31BIT|MEMF_24BITDMA},
    /*
     * FIXME: The following should also be CHIP. trackdisk.device fix is needed
     * (use MEMF_24BITDMA instead of MEMF_CHIP for 24-bit ISA DMA-capable area.
     * EXPERIMENTAL: Some (or all?) 64-bit machines expose RAM at addresses up to
     * 0xD0000000 (giving 3.5 GB total). All MMIO sits beyond this border. We
     * intentionally specify 4GB as limit, just in case if some machine exhibits
     * even more RAM in this space. We want all the RAM to be usable.
     */
    {0x01000000, 0xFFFFFFFF, "High memory"   ,  0, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_FAST|MEMF_31BIT	      },
    {0         , 0         , NULL            ,  0, 0                                                                  }
};

/*
 * Our transient data.
 * They must survive warm restart, so we put them into .data section.
 * We also have SysBase here, this way we move it away from zero page,
 * making it harder to trash it.
 */
__attribute__((section(".data"))) IPTR kick_end = 0;
__attribute__((section(".data"))) struct segment_desc *GDT = NULL;
__attribute__((section(".data"))) struct ExecBase *SysBase = NULL;

/*
 * Static read-only copy of prebuilt GDT.
 * We only need to patch a TSS segment, after TSS has been allocated.
 */
static const struct {UWORD l1, l2, l3, l4;}
    GDT_Table[] = 
{
        { 0x0000, 0x0000, 0x0000, 0x0000 },
        { 0xffff, 0x0000, 0x9a00, 0x00cf },
        { 0xffff, 0x0000, 0x9200, 0x00cf },
        { 0xffff, 0x0000, 0xfa00, 0x00cf },
        { 0xffff, 0x0000, 0xf200, 0x00cf },
        { 0x0000, 0x0000, 0x0000, 0x0000 },
        { 0x0067, 0x0000, 0x8900, 0x0000 },
        { 0x0000, 0x0000, 0x0000, 0x0000 }
};

/*
 * This is the main entry point.
 * We run from here both at first boot and upon reboot.
 */
void kernel_cstart(const struct TagItem *msg)
{
    struct TagItem *tag;
    struct mb_mmap *mmap = NULL;
    unsigned long mmap_len = 0;
    IPTR kick_start = 0;
    struct table_desc gdtr;
    struct MinList memList;
    struct MemHeader *mh, *mh2;
    UWORD *ranges[3];
    struct mb_mmap *region;
    char *cmdline = NULL;
    ULONG allocator = ALLOCATOR_TLSF;

    D(bug("[Kernel] Transient kickstart end 0x%p, BootMsg 0x%p\n", kick_end, BootMsg));
    D(bug("[Kernel] Boot stack: 0x%p - 0x%p\n", boot_stack, boot_stack + STACK_SIZE));

    if (!kick_end)
    {
        struct vbe_mode *vmode = NULL;

    	/* If kick_end is not set, this is our first start. */
	tag = LibFindTagItem(KRN_KernelHighest, msg);
        if (!tag)
	    krnPanic(KernelBase, "Incomplete information from the bootstrap\n"
		     "Highest kickstart address is not supplied\n");

        /* Align kickstart top address (we are going to place a structure after it) */
        BootMemPtr = (void *)AROS_ROUNDUP2(tag->ti_Data + 1, sizeof(APTR));

	/*
	 * Our boot taglist is placed by the bootstrap just somewhere in memory.
	 * The first thing is to move it into some safe place.
	 * This function also sets global BootMsg pointer.
	 */
	RelocateBootMsg(msg);

	/* Now relocate linked data */
	mmap_len = LibGetTagData(KRN_MMAPLength, 0, BootMsg);
	msg = BootMsg;
	while ((tag = LibNextTagItem((struct TagItem **)&msg)))
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

	/* Allocate space for GDT */
	GDT = krnAllocBootMemAligned(sizeof(GDT_Table), 128);

	vesahack_Init(cmdline, vmode);

	/*
	 * Set new kickstart end address.
	 * Kickstart area now includes boot taglist with all its contents.
	 */
	kick_end = (IPTR)BootMemPtr;
	D(bug("[Kernel] Boot-time setup complete, end of kickstart area 0x%p\n", kick_end));
    }

    /*
     * Obtain the needed data from the boot taglist.
     * We parse it from scratch here because we come here in both cases
     * (first boot and reboot)
     */
    msg = BootMsg;
    while ((tag = LibNextTagItem((struct TagItem **)&msg)))
    {
        switch (tag->ti_Tag)
        {
        case KRN_KernelLowest:
            kick_start = tag->ti_Data;
            break;

        case KRN_MMAPAddress:
            mmap = (struct mb_mmap *)tag->ti_Data;
            break;

        case KRN_MMAPLength:
            mmap_len = tag->ti_Data;
            break;

        case KRN_CmdLine:
            cmdline = (char *)tag->ti_Data;
            break;
        }
    }

    /* Sanity check */
    if ((!kick_start) || (!mmap) || (!mmap_len))
    {
	krnPanic(KernelBase, "Incomplete information from the bootstrap\n"
		 "Kickstart address : 0x%P\n"
		 "Memory map address: 0x%P, length %ld\n",
		 kick_start, mmap, mmap_len);
    }

    if (cmdline && strstr(cmdline, "notlsf"))
        allocator = ALLOCATOR_STD;


    /* Create global descriptor table */
    krnCopyMem(GDT_Table, GDT, sizeof(GDT_Table));

    /*
     * Initial CPU setup. Load the GDT and segment registers.
     * AROS uses only CS SS DS and ES. FS and GS are set to 0
     * so we can generate GP if someone uses them.
     */
    gdtr.size = sizeof(GDT_Table) - 1;
    gdtr.base = (unsigned long)GDT;
    asm
    (
	"	lgdt	%0\n"
	"	mov    	%1,%%ds\n"
	"	mov    	%1,%%es\n"
	"	mov    	%1,%%ss\n"
	"	mov    	%2,%%fs\n"
	"	mov    	%2,%%gs\n"
	"	ljmp   	%3,$1f\n"
	"1:\n"
	::"m"(gdtr),"r"(KERNEL_DS),"r"(0),"i"(KERNEL_CS)
    );

    D(bug("[Kernel] GDT @ 0x%p reloaded\n", GDT));

    /*
     * Explore memory map and create MemHeaders
     * 4KB at address 0 are reserved for our needs.
     */
    NEWLIST(&memList);
    mmap_InitMemory(mmap, mmap_len, &memList, kick_start, kick_end, 0x00001000, PC_Memory, allocator);



    /*
     * mmap_InitMemory() adds MemHeaders to the list in the order they were created.
     * I. e. highest addresses are added last.
     * Take highest region in order to create SysBase in it.
     */
    mh = (struct MemHeader *)REMTAIL(&memList);


    D(bug("[Kernel] Initial MemHeader: 0x%p - 0x%p (%s)\n", mh->mh_Lower, mh->mh_Upper, mh->mh_Node.ln_Name));

    if (SysBase)
    {
	/*
	 * Validate SysBase.
	 * Criteria: The pointer should point to a valid memory region.
	 * This is only address validation.
	 * Checksum etc is processed in PrepareExecBase() in exec.library.
	 */
	BOOL sysbase_bad = TRUE;

	region = mmap_FindRegion((unsigned long)SysBase, mmap, mmap_len);
	if (region && region->type == MMAP_TYPE_RAM)
	{
	    IPTR end = region->addr + region->len;

	    if ((IPTR)SysBase + sizeof(struct ExecBase) < end)
		sysbase_bad = FALSE;
	}

	if (sysbase_bad)
	    SysBase = NULL;
    }

    ranges[0] = (UWORD *)kick_start;
    ranges[1] = (UWORD *)kick_end;
    ranges[2] = (UWORD *)-1;

    krnPrepareExecBase(ranges, mh, BootMsg);

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

    D(bug("[Kernel] Created SysBase at 0x%p, MemHeader 0x%p\n", SysBase, mh));

    /* Transfer the rest of memory list into SysBase */
    D(bug("[Kernel] Transferring memory list into SysBase...\n"));
    for (mh = (struct MemHeader *)memList.mlh_Head; mh->mh_Node.ln_Succ; mh = mh2)
    {
        mh2 = (struct MemHeader *)mh->mh_Node.ln_Succ;

	D(bug("[Kernel] * 0x%p - 0x%p (%s)\n", mh->mh_Lower, mh->mh_Upper, mh->mh_Node.ln_Name));
	Enqueue(&SysBase->MemList, &mh->mh_Node);
    }
    
    /*
     * Now we can initialize SINGLETASK residents.
     * This includes kernel.resource itself. Its platform-specific code
     * will initialize the rest of hardware.
     */
    InitCode(RTF_SINGLETASK, 0);

    /*
     * After RTF_SINGLETASK we can have various interesting things like ACPI.
     * Secondary platform initialization code makes use of them.
     */
    PlatformPostInit();

#    define _stringify(x) #x
#    define stringify(x) _stringify(x)

    asm("movl $" stringify(USER_DS) ",%%eax\n\t"
        "mov %%eax,%%ds\n\t"                         /* User DS */
	"mov %%eax,%%es\n\t"                         /* User ES */
	"movl %%esp,%%ebx\n\t"			/* Hold the esp value before pushing! */
	"pushl %%eax\n\t"                           /* User SS */
	"pushl %%ebx\n\t"                           /* Stack frame */
	"pushl $0x3002\n\t"                        /* IOPL:3 */
	"pushl $" stringify(USER_CS) "\n\t"        /* User CS */
	"pushl $1f\n\t"                            /* Entry address */
	"iret\n"                                   /* Go down to the user mode */
	"1:":::"eax","ebx");

    InitCode(RTF_COLDSTART, 0);
	
    krnPanic(KernelBase, "Failed to start up the system");
}

/* Our boot-time stack. Safe to be in .bss. */
static char boot_stack[STACK_SIZE] __attribute__((aligned(16)));
