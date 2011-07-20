#define MDEBUG 1

#include <aros/atomic.h>
#include <aros/debug.h>
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

#include "kernel_base.h"
#include "kernel_bootmem.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "kernel_memory.h"
#include "kernel_romtags.h"
#include "acpi.h"
#include "apic.h"
#include "tls.h"

/* Common IBM PC memory layout */
static const struct MemRegion PC_Memory[] =
{
    {0x000000000, 0x000100000, "Low memory"    , -5, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_31BIT|MEMF_CHIP|MEMF_24BITDMA},
    {0x000100000, 0x001000000, "ISA DMA memory", -5, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_31BIT|MEMF_CHIP|MEMF_24BITDMA},
    {0x001000000, 0x080000000, "31-bit memory" ,  0, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_31BIT			},
    {0x080000000, -1         , "Upper memory"  , 10, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK                                   },
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
 * This address is selected for console mirror because:
 * a) The bootstrap is placed at 0x1000000. It has already done its job and we can reuse its memory.
 * b) 0x1000 is added in order not to overlap with MemHeader and first MemChunk. We are running on a
 *    PC, so we know, that a new physical memory region always starts at 0x100000 (1MB). When memory map
 *    is converted into exec lists by mmap_InitMemory(), a MemHeader and first MemChunk will be placed in
 *    the start of the region. 4KB is more than enough not to clash with it.
 */
static void boot_start(struct TagItem *msg)
{
    fb_Mirror = (void *)0x101000;
    con_InitTagList(msg);

    bug("AROS64 - The AROS Research OS, 64-bit version. Compiled %s\n", __DATE__);
    D(bug("[Kernel] kernel_cstart: Jumped into kernel.resource @ %p [stub @ %p].\n", boot_start, start64));

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

    /* ... then switch to initial stack and jump to target address */
    asm volatile("movq %1, %%rsp\n\t"
    		 "jmp *%2\n"::"D"(msg), "r"(boot_stack + STACK_SIZE - 16), "r"(target));
}

/* Print panic string and halt */
static void panic(void)
{
    bug("*** SYSTEM PANIC ***\n");
    while (1) asm volatile("hlt");
}

static void RelocateTagData(struct TagItem *tag, unsigned long size)
{
    char *src = (char *)tag->ti_Data;
    unsigned char *dest = krnAllocBootMem(size);
    unsigned int i;

    tag->ti_Data = (IPTR)dest;

    /* Do not use memcpy() because it can rely on CopyMem() which is not available yet */
    for (i = 0; i < size; i++)
        *dest++ = *src++;
}

/* All CPUs start up from this point */
void kernel_cstart(const struct TagItem *msg)
{
    IPTR _APICBase;
    UBYTE _APICID;

    /* Enable fxsave/fxrstor */ 
    wrcr(cr4, rdcr(cr4) | _CR4_OSFXSR | _CR4_OSXMMEXCPT);

    D(bug("[Kernel] Boot data: 0x%p\n", __KernBootPrivate));
    if (__KernBootPrivate == NULL)
    {
    	/* This is our first start. */
	struct TagItem *tag = LibFindTagItem(KRN_KernelHighest, msg);
	struct TagItem *dest;
	unsigned long mlen;
        IPTR ptr;
        struct vbe_mode *vmode = NULL;
        char *cmdline = NULL;

        if (!tag)
        {
            bug("Incomplete information from the bootstrap\n");
            bug("Highest kickstart address is not supplied\n");

            panic();
        }

        /* Align kickstart top address (we are going to place a structure after it) */
        ptr = AROS_ROUNDUP2(tag->ti_Data + 1, sizeof(APTR));

	memset((void *)ptr, 0, sizeof(struct KernBootPrivate));
        __KernBootPrivate = (struct KernBootPrivate *)ptr;

	/*
	 * Our boot taglist is placed by the bootstrap just somewhere in memory.
	 * The first thing is to move it into some safe place.
	 */
        ptr = AROS_ROUNDUP2(ptr + sizeof(struct KernBootPrivate), sizeof(APTR));
        BootMsg = (struct TagItem *)ptr;

	dest = BootMsg;
        while ((tag = LibNextTagItem(&msg)))
    	{
    	    dest->ti_Tag  = tag->ti_Tag;
    	    dest->ti_Data = tag->ti_Data;
    	    dest++;
    	}
	dest->ti_Tag = TAG_DONE;
	dest++;

	__KernBootPrivate->kbp_PrivateNext = (IPTR)dest;

	/* Now relocate linked data */
	mlen = LibGetTagData(KRN_MMAPLength, 0, BootMsg);
	msg = BootMsg;
	while ((tag = LibNextTagItem(&msg)))
    	{
    	    unsigned long l;
    	    struct KernelBSS *bss;

    	    switch (tag->ti_Tag)
    	    {
    	    case KRN_KernelBss:
    	    	l = sizeof(struct KernelBSS);
    	    	for (bss = (struct KernelBSS *)tag->ti_Data; bss->addr; bss++)
    	    	    l += sizeof(struct KernelBSS);

    	    	RelocateTagData(tag, l);
    	    	break;

    	    case KRN_MMAPAddress:
    	    	RelocateTagData(tag, mlen);
    	    	break;

    	    case KRN_VBEModeInfo:
    	    	RelocateTagData(tag, sizeof(struct vbe_mode));
    	    	vmode = (struct vbe_mode *)tag->ti_Data;
    	    	break;

    	    case KRN_VBEControllerInfo:
    	    	RelocateTagData(tag, sizeof(struct vbe_controller));
    	    	break;

	    case KRN_CmdLine:
	    	l = strlen((char *)tag->ti_Data) + 1;
	    	RelocateTagData(tag, l);
	    	cmdline = (char *)tag->ti_Data;
	    	break;
	    }
	}

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

    D(bug("[Kernel] End of kickstart data area: 0x%p\n", __KernBootPrivate->kbp_PrivateNext));

    if (!(__KernBootPrivate->kbp_InitFlags & KERNBOOTFLAG_BOOTCPUSET))
    {
    	D(bug("[Kernel] Setting boot CPU...\n"));

        core_APICProbe(__KernBootPrivate);

        _APICBase = boot_APIC_GetBase(__KernBootPrivate);
        _APICID   = boot_APIC_GetID(__KernBootPrivate, _APICBase);

        D(bug("[Kernel] kernel_cstart[%d]: launching on BSP APIC ID %d, base @ %p\n", _APICID, _APICID, _APICBase));
        D(bug("[Kernel] kernel_cstart[%d]: KernelBootPrivate @ %p [%d bytes], Next @ %p\n", _APICID, __KernBootPrivate, sizeof(struct KernBootPrivate), __KernBootPrivate->kbp_PrivateNext));

        __KernBootPrivate->kbp_APIC_BSPID = _APICID;
        __KernBootPrivate->kbp_InitFlags |= KERNBOOTFLAG_BOOTCPUSET;

        /* Initialize the ACPI boot-time table parser. */
        __KernBootPrivate->kbp_ACPIRSDP = core_ACPIProbe(msg, __KernBootPrivate);
        bug("[Kernel] kernel_cstart[%d]: core_ACPIProbe() returned %p\n", _APICID, __KernBootPrivate->kbp_ACPIRSDP);
    }
    else
    {
        _APICBase = boot_APIC_GetBase(__KernBootPrivate);
        _APICID   = boot_APIC_GetID(__KernBootPrivate, _APICBase);

        bug("[Kernel] kernel_cstart[%d]: launching on AP APIC ID %d, base @ %p\n", _APICID, _APICID, _APICBase);
        bug("[Kernel] kernel_cstart[%d]: KernelBootPrivate @ %p\n", _APICID, __KernBootPrivate);
    }
    /* Prepare GDT */
    core_SetupGDT(__KernBootPrivate);

    /* Set TSS, GDT, LDT and MMU up */
    core_CPUSetup(_APICBase);
    core_SetupIDT(__KernBootPrivate);
    core_SetupMMU(__KernBootPrivate);

    D(bug("[Kernel] kernel_cstart[%d]: APIC_BASE_MSR=%016p\n", _APICID, _APICBase + 0x900));

    if (_APICID == __KernBootPrivate->kbp_APIC_BSPID)
    {
    	/*
    	 * This thread is running on a primary CPU.
    	 * Let's initialize the system now. Other CPUs will be launched
    	 * in kernel.resource normal init routine (kernel_init.c)
    	 */
        struct MinList memList;
        struct MemHeader *mh, *mh2;
        struct mb_mmap *mmap = NULL;
        IPTR mmap_len = 0;
        IPTR addr = 0;
        IPTR klo  = 0;
        IPTR khi  = AROS_ROUNDUP2(__KernBootPrivate->kbp_PrivateNext, PAGE_SIZE);
        const struct TagItem *tstate = BootMsg;
        struct TagItem *tag;
        UWORD *ranges[] = {NULL, NULL, (UWORD *)-1};

	D(bug("[Kernel] kernel_cstart[%d] Launching on BSP\n", _APICID));

	/* Obtain the needed data from the boot taglist */
        while ((tag = LibNextTagItem(&tstate)))
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

        /* Setup the 8259 */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x11),"i"(0x20)); /* Initialization sequence for 8259A-1 */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x11),"i"(0xa0)); /* Initialization sequence for 8259A-2 */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x20),"i"(0x21)); /* IRQs at 0x20 - 0x27 */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x28),"i"(0xa1)); /* IRQs at 0x28 - 0x2f */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x04),"i"(0x21)); /* 8259A-1 is master */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x02),"i"(0xa1)); /* 8259A-2 is slave */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x01),"i"(0x21)); /* 8086 mode for both */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x01),"i"(0xa1));
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0xff),"i"(0x21)); /* Enable cascade int */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0xff),"i"(0xa1)); /* Mask all interrupts */

        D(bug("[Kernel] kernel_cstart: Interrupts redirected. We will go back in a minute ;)\n"));

	/* Explore memory map and create MemHeaders */
	NEWLIST(&memList);
        mmap_InitMemory(mmap, mmap_len, &memList, klo, khi, PC_Memory);

        D(bug("[Kernel] kernel_cstart: Booting exec.library...\n"));

	/* Create SysBase in the first MemHeader (low memory) */
	mh  = (struct MemHeader *)memList.mlh_Head;
	/*
	 * Take next MemHeader because PrepareExecBase() will insert
	 * the first one into exec memory list and destroy its original
	 * next/pred pointers
	 */
	mh2 = (struct MemHeader *)mh->mh_Node.ln_Succ;

	/*
	 * Fill zero page with garbage in order to detect accesses to it in supervisor mode.
	 * For user mode we will disable all access to it.
	 */
	MUNGE_BLOCK(NULL, 0xABADCAFE, PAGE_SIZE);

	/*
	 * TODO: We may have SysBase validation code instead of this.
	 * This will let us to keep KickTags accross reboots.
	 */
        SysBase = NULL;

	ranges[0] = (UWORD *)klo;
	ranges[1] = (UWORD *)khi;
        if (!krnPrepareExecBase(ranges, mh, BootMsg))
        {
            bug("Failed to create exec.library base\n");

            panic();
        }

	D(bug("[Kernel] Created SysBase at 0x%p (pointer at 0x%p), MemHeader 0x%p\n", SysBase, &SysBase, mh));

	/* Block all access to zero page */
	core_ProtKernelArea(0, PAGE_SIZE, 0, 0, 0);

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
	__KernBootPrivate->kbp_LowMem = mh;
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
	while (mh2->mh_Node.ln_Succ)
	{
            mh = (struct MemHeader *)mh2->mh_Node.ln_Succ;

	    D(bug("[Kernel] * 0x%p - 0x%p (%s)\n", mh2->mh_Lower, mh2->mh_Upper, mh2->mh_Node.ln_Name));
	    Enqueue(&SysBase->MemList, &mh2->mh_Node);

	    mh2 = mh;
	}

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

	InitCode(RTF_SINGLETASK, 0);
	InitCode(RTF_COLDSTART, 0);

	bug("[Kernel] ERROR: System Boot Failed!\n");
	panic();
    }
    else
    {
	/*
	 * This is a secondary CPU. Currently it has nothing to do :-(
	 * KernelBase is already set up by the primary core, so we can use it.
	 */
        UBYTE _APICNO = core_APICGetNumber(KernelBase->kb_PlatformData);

        AROS_ATOMIC_INC(KernelBase->kb_PlatformData->kb_APIC_Ready);
        bug("[Kernel] APIC No. %d of %d Going IDLE (Halting)...\n", _APICNO, KernelBase->kb_PlatformData->kb_APIC_Count);
    }

    while (1) asm volatile("hlt");
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

struct gdt_selector
{
    uint16_t size;
    uint64_t base;
} __attribute__((packed));

void core_SetupGDT(struct KernBootPrivate *__KernBootPrivate)
{
    IPTR  _APICBase;
    UBYTE _APICID;
    int i;

    _APICBase = boot_APIC_GetBase(__KernBootPrivate);
    _APICID   = boot_APIC_GetID(__KernBootPrivate, _APICBase);

    D(bug("[Kernel] core_SetupGDT(%d)\n", _APICID));

    if (_APICID == __KernBootPrivate->kbp_APIC_BSPID)
    {
    	struct gdt_64bit *GDT;
    	struct tss_64bit *TSS;
    	intptr_t tls_ptr;

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
            GDT->tss[i].tss_low.type       = 0x09;				      	/* 64-bit TSS */
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

        GDT->gs.type=0x12;      /* data segment */
        GDT->gs.dpl=3;    	/* user level */
        GDT->gs.p=1;            /* present */
        GDT->gs.base_low = tls_ptr & 0xffff;
        GDT->gs.base_mid = (tls_ptr >> 16) & 0xff;
        GDT->gs.base_high = (tls_ptr >> 24) & 0xff;   
        GDT->gs.g=1;
        GDT->gs.d=1;
    }
}
 
void core_CPUSetup(IPTR _APICBase)
{
    struct segment_selector GDT_sel;
    struct tss_64bit *TSS = __KernBootPrivate->TSS;
    UBYTE _APICID = boot_APIC_GetID(__KernBootPrivate, _APICBase);

    D(bug("[Kernel] core_CPUSetup[%d]\n", _APICID));

    if (_APICID == __KernBootPrivate->kbp_APIC_BSPID)
    {
    	/* 
    	 * Allocate out supervisor stack from boot-time memory.
    	 * It will be protected from user's intervention.
    	 * Allocate actually three stacks: panic, supervisor, ring1.
    	 * Note that we do the actual allocation only once. The region is kept
    	 * in __KernBootPrivate which survives warm reboots.
     	 */
     	if (!__KernBootPrivate->SystemStack)
    	    __KernBootPrivate->SystemStack = (IPTR)krnAllocBootMem(STACK_SIZE * 3);
    }
    /*
     * FIXME: Other CPUs should have own supervisor stacks. They can't allocate them because:
     * 1. __KernBootPrivate is already sealed up. The memory behind it is given up
     *    to the OS.
     * 2. AllocMem() is not SMP-aware.
     *
     * For now this is the same as it was - all CPUs reuse the same stacks. Likely
     * this would cause crash if we attempt to use more than one CPU.
     * In fact the bootstrap CPU could allocate these stacks before actually running these CPUs.
     */

    TSS[_APICID].ist1 = __KernBootPrivate->SystemStack + STACK_SIZE     - 16;
    TSS[_APICID].rsp0 = __KernBootPrivate->SystemStack + STACK_SIZE * 2 - 16;
    TSS[_APICID].rsp1 = __KernBootPrivate->SystemStack + STACK_SIZE * 3 - 16;

    bug("[Kernel] core_CPUSetup[%d]: Reloading the GDT and Task Register\n", _APICID);

    GDT_sel.size = sizeof(struct gdt_64bit) - 1;
    GDT_sel.base = (uint64_t)__KernBootPrivate->GDT;
    asm volatile ("lgdt %0"::"m"(GDT_sel));
    asm volatile ("ltr %w0"::"r"(TASK_SEG + (_APICID << 4)));
    asm volatile ("mov %0,%%gs"::"a"(USER_GS));
}
