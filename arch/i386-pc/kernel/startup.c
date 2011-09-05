#include <aros/altstack.h>
#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <aros/symbolsets.h>
#include <proto/arossupport.h>

#include <strings.h>
#include <inttypes.h>

#include "boot_utils.h"
#include "kernel_base.h"
#include "kernel_bootmem.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "kernel_mmap.h"
#include "kernel_romtags.h"

#define D(x) x

static char boot_stack[];

kerncall void start32(struct TagItem *msg);
static void core_Kick(struct TagItem *msg, void *target);
static void kernel_boot(const struct TagItem *msg);
static void kernel_cstart(const struct TagItem *msg);

/* Some code is still in exec.library */
void clr(void);
void exec_cinit(void);
void exec_boot(struct TagItem *msg);

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

static void kernel_boot(const struct TagItem *msg)
{
    clr();
    bug("AROS - The AROS Research OS\nCompiled %s\n\n",__DATE__);
    
    kernel_cstart(msg);
}

/*
 * The second important place. We come here upon warm restart,
 * with stack pointer set to some safe place.
 */
void kernel_reboot(void)
{
    core_Kick(BootMsg, kernel_cstart);
}

/*
 * This function actually runs the kickstart from the specified address.
 * Before doing this it clears .bss sections of all modules.
 */
static void core_Kick(struct TagItem *msg, void *target)
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

static void panic(void)
{
    bug("*** SYSTEM PANIC ***\n");
    while (1);
}

/* Common IBM PC memory layout */
static const struct MemRegion PC_Memory[] =
{
    /*
     * Give low memory a bit lower priority. This will help us to locate its MemHeader (the last one in the list).
     * We explicitly need low memory for SMP bootstrap.
     */
    {0x00000000, 0x00100000, "Low memory"    , -6, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_CHIP|MEMF_31BIT|MEMF_24BITDMA},
    {0x00100000, 0x01000000, "ISA DMA memory", -5, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_CHIP|MEMF_31BIT|MEMF_24BITDMA},
    /*
     * FIXME: The following should also be CHIP. trackdisk.device fix is needed
     * (use MEMF_24BITDMA instead of MEMF_CHIP for 24-bit ISA DMA-capable area.
     */
    {0x01000000, 0x80000000, "High memory"   ,  0, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_FAST|MEMF_31BIT	      },
    {0         , 0         , NULL            ,  0, 0                                                                  }
};

/*
 * Our transient data.
 * They must survive warm restart, so we put then into .data section.
 */
__attribute__((section(".data"))) static IPTR kick_end = 0;

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
    struct MinList memList;
    struct MemHeader *mh, *mh2;
    UWORD *ranges[3];
    struct mb_mmap *region;

    D(bug("[Kernel] Transient BootMsg: 0x%p\n", BootMsg));
    D(bug("[Kernel] Boot stack: 0x%p - 0x%p\n", boot_stack, boot_stack + STACK_SIZE));

    if (!kick_end)
    {
    	/* If kick_end is not set, this is our first start. */
	void *ptr;

	tag = LibFindTagItem(KRN_KernelHighest, msg);
        if (!tag)
        {
            bug("Incomplete information from the bootstrap\n");
            bug("Highest kickstart address is not supplied\n");

            panic();
        }

        /* Align kickstart top address (we are going to place a structure after it) */
        ptr = (void *)AROS_ROUNDUP2(tag->ti_Data + 1, sizeof(APTR));

	/*
	 * Our boot taglist is placed by the bootstrap just somewhere in memory.
	 * The first thing is to move it into some safe place.
	 * This function also sets global BootMsg pointer.
	 */
	BootMemPtr = RelocateBootMsg(msg, ptr);

	/* Now relocate linked data */
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
    	    	break;

    	    case KRN_VBEControllerInfo:
    	    	RelocateTagData(tag, sizeof(struct vbe_controller));
    	    	break;

	    case KRN_CmdLine:
	    	RelocateStringData(tag);
	    	break;

	    case KRN_BootLoader:
	    	RelocateStringData(tag);
	    	break;
	    }
	}

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
    while ((tag = LibNextTagItem(&msg)))
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
        }
    }

    /* Sanity check */
    if ((!kick_start) || (!mmap) || (!mmap_len))
    {
        bug("Incomplete information from the bootstrap\n");
	bug("Kickstart address : 0x%P\n", kick_start);
        bug("Memory map address: 0x%P, length %ld\n", mmap, mmap_len);

        panic();
    }

    D(bug("[Kernel] Booting exec.library...\n"));
    exec_cinit();

    /*
     * Explore memory map and create MemHeaders
     * 2KB at address 0 are reserved for our needs.
     */
    NEWLIST(&memList);
    mmap_InitMemory(mmap, mmap_len, &memList, kick_start, kick_end, 0x00002000, PC_Memory);

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

    if (!krnPrepareExecBase(ranges, mh, BootMsg))
    {
        bug("Failed to create exec.library base\n");

        panic();
    }

    D(bug("[Kernel] Created SysBase at 0x%p, MemHeader 0x%p\n", SysBase, mh));

    /*
     * Boot task skeleton is created by PrepareExecBase().
     * Fill in stack limits.
     */
    SysBase->ThisTask->tc_SPLower = boot_stack;
    SysBase->ThisTask->tc_SPUpper = boot_stack + STACK_SIZE;
    aros_init_altstack(SysBase->ThisTask);

    /* Transfer the rest of memory list into SysBase */
    D(bug("[Kernel] Transferring memory list into SysBase...\n"));
    for (mh = (struct MemHeader *)memList.mlh_Head; mh->mh_Node.ln_Succ; mh = mh2)
    {
        mh2 = (struct MemHeader *)mh->mh_Node.ln_Succ;

	D(bug("[Kernel] * 0x%p - 0x%p (%s)\n", mh->mh_Lower, mh->mh_Upper, mh->mh_Node.ln_Name));
	Enqueue(&SysBase->MemList, &mh->mh_Node);
    }

    exec_boot(BootMsg);
}

/* Our boot-time stack. Safe to be in .bss. */
static char boot_stack[STACK_SIZE] __attribute__((aligned(16)));
