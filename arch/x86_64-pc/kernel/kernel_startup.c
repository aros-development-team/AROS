#define DEBUG 1

#include <aros/debug.h>
#include <aros/multiboot.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <asm/segments.h>
#include <inttypes.h>
#include <aros/symbolsets.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/resident.h>

#include <utility/tagitem.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <bootconsole.h>
#include <stdio.h>
#include <stdlib.h>

#include "kernel_intern.h"
#include "kernel_memory.h"

extern const unsigned long start64;

struct ExecBase *krnPrepareExecBase(UWORD *ranges[], struct MemHeader *mh, struct TagItem *bootMsg);

/* Common IBM PC memory layout */
static const struct MemRegion PC_Memory[] =
{
    {0x000000000, 0x000100000, "Low memory"    , -5, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_31BIT|MEMF_CHIP|MEMF_24BITDMA},
    {0x000100000, 0x001000000, "ISA DMA memory", -5, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_31BIT|MEMF_CHIP|MEMF_24BITDMA},
    {0x001000000, 0x100000000, "32-bit memory" ,  0, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|MEMF_31BIT			},
    {0x100000000, -1         , "Upper memory"  , 10, MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK                                   },
    {0          , 0          , NULL            ,  0, 0                                                                  }
};

/* Pre-exec init */

asm(".section .aros.init,\"ax\"\n\t"
    ".globl start64\n\t"
    ".type start64,@function\n"
    "start64: movq tmp_stack_end(%rip),%rsp\n\t"
    "movq %rdi,%rbx\n\t"
    "call __clear_bss\n\t"
    "movq %rbx,%rdi\n\t"
    "movq stack_end(%rip), %rsp\n\t"
    "movq target_address(%rip), %rsi\n\t"
    "jmp *%rsi\n\t"
    ".string \"Native/CORE v3 (" __DATE__ ")\""
    "\n\t.text\n\t"
);

struct KernBootPrivate *__KernBootPrivate = NULL;

void __clear_bss(struct TagItem *msg)
{
    struct KernelBSS *bss = (struct KernelBSS *)LibGetTagData(KRN_KernelBss, 0, msg);

    if (bss)
    {
        while (bss->addr)
        {
            bzero((void *)bss->addr, bss->len);
            bss++;
        }
    }
}

/* Print panic string and halt */
static void panic(void)
{
    bug("*** SYSTEM PANIC ***\n");
    while (1) asm volatile("hlt");
}

static void *RelocateTagData(unsigned char *dest, struct TagItem *tag, unsigned long size)
{
    char *src = (char *)tag->ti_Data;
    unsigned int i;

    tag->ti_Data = (IPTR)dest;

    /* Do not use memcpy() because it can rely on CopyMem() which is not available yet */
    for (i = 0; i < size; i++)
        *dest++ = *src++;

    /* Align the next address */
    return (void *)AROS_ROUNDUP2((IPTR)dest, sizeof(APTR));
}

static struct TagItem *BootMsg;
static volatile UBYTE apicready = 0;

void kernel_cstart(struct TagItem *msg, void *entry)
{
    IPTR _APICBase;
    UBYTE _APICID;

    /*
     * Initialize console ASAP in order to get debug output correctly.
     * This will deal with both serial and on-screen console.
     */
    fb_Mirror = (void *)0x101000;
    con_InitTagList(msg);

    bug("AROS64 - The AROS Research OS, 64-bit version. Compiled %s\n", __DATE__);
    D(bug("[Kernel] kernel_cstart: Jumped into kernel.resource @ %p [asm stub @ %p].\n", kernel_cstart, &start64));

    /* Enable fxsave/fxrstor */ 
    wrcr(cr4, rdcr(cr4) | _CR4_OSFXSR | _CR4_OSXMMEXCPT);

    D(bug("[Kernel] Boot data: 0x%p\n", __KernBootPrivate));
    if (__KernBootPrivate == NULL)
    {
    	/* This is our first start. */
	struct TagItem *tag = LibFindTagItem(KRN_KernelHighest, msg);
	struct TagItem *dest;
	unsigned long mlen;
        void *ptr;
        
        if (!tag)
        {
            bug("Incomplete information from the bootstrap\n");
            bug("Highest kickstart address is not supplied\n");

            panic();
        }

        /* Align kickstart top address (we are going to place a structure after it) */
        ptr = (void *)AROS_ROUNDUP2(tag->ti_Data + 1, sizeof(APTR));

        __KernBootPrivate = ptr;
        __KernBootPrivate->kbp_InitFlags = 0;

	/*
	 * Our boot taglist is placed by the bootstrap just somewhere in memory.
	 * The first thing is to move it into some safe place.
	 */
        BootMsg = ptr + sizeof(struct KernBootPrivate);

	dest = BootMsg;
        while ((tag = LibNextTagItem(&msg)))
    	{
    	    dest->ti_Tag  = tag->ti_Tag;
    	    dest->ti_Data = tag->ti_Data;
    	    dest++;
    	}
	dest->ti_Tag = TAG_DONE;
	ptr = dest + 1;	

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

    	    	ptr = RelocateTagData(ptr, tag, l);
    	    	break;

    	    case KRN_MMAPAddress:
    	    	ptr = RelocateTagData(ptr, tag, mlen);
    	    	break;

    	    case KRN_VBEModeInfo:
    	    	ptr = RelocateTagData(ptr, tag, sizeof(struct vbe_mode));
    	    	break;

    	    case KRN_VBEControllerInfo:
    	    	ptr = RelocateTagData(ptr, tag, sizeof(struct vbe_controller));
    	    	break;

	    case KRN_CmdLine:
	    	l = strlen((char *)tag->ti_Data);
	    	ptr = RelocateTagData(ptr, tag, l);
	    	break;
	    }
	}
	__KernBootPrivate->kbp_PrivateNext = (IPTR)ptr;
    }

    D(bug("[Kernel] End of kickstart data area: 0x%p\n", __KernBootPrivate->kbp_PrivateNext));

    if (!(__KernBootPrivate->kbp_InitFlags & KERNBOOTFLAG_BOOTCPUSET))
    {
    	D(bug("[Kernel] Setting boot CPU...\n"));

        core_APICProbe(__KernBootPrivate);

        _APICBase = AROS_UFC0(IPTR,
                ((struct GenericAPIC *)__KernBootPrivate->kbp_APIC_Drivers[__KernBootPrivate->kbp_APIC_DriverID])->getbase);
        _APICID = (UBYTE)AROS_UFC1(IPTR,
                ((struct GenericAPIC *)__KernBootPrivate->kbp_APIC_Drivers[__KernBootPrivate->kbp_APIC_DriverID])->getid,
                        AROS_UFCA(IPTR, _APICBase, A0));

        D(bug("[Kernel] kernel_cstart[%d]: launching on BSP APIC ID %d, base @ %p\n", _APICID, _APICID, _APICBase));
        D(bug("[Kernel] kernel_cstart[%d]: KernelBootPrivate @ %p [%d bytes], Next @ %p\n", _APICID, __KernBootPrivate, sizeof(struct KernBootPrivate), __KernBootPrivate->kbp_PrivateNext));

        __KernBootPrivate->kbp_APIC_BSPID = _APICID;
        __KernBootPrivate->kbp_InitFlags |= KERNBOOTFLAG_BOOTCPUSET;

        /* Initialize the ACPI boot-time table parser. */
        __KernBootPrivate->kbp_ACPIRSDP = core_ACPIProbe(msg, __KernBootPrivate);
        rkprintf("[Kernel] kernel_cstart[%d]: core_ACPIProbe() returned %p\n", _APICID, __KernBootPrivate->kbp_ACPIRSDP);
    }
    else
    {
        _APICBase = AROS_UFC0(IPTR,
                ((struct GenericAPIC *)__KernBootPrivate->kbp_APIC_Drivers[__KernBootPrivate->kbp_APIC_DriverID])->getbase);
        _APICID = (UBYTE)AROS_UFC1(IPTR,
                ((struct GenericAPIC *)__KernBootPrivate->kbp_APIC_Drivers[__KernBootPrivate->kbp_APIC_DriverID])->getid,
                        AROS_UFCA(IPTR, _APICBase, A0));

        rkprintf("[Kernel] kernel_cstart[%d]: launching on AP APIC ID %d, base @ %p\n", _APICID, _APICID, _APICBase);
        rkprintf("[Kernel] kernel_cstart[%d]: KernelBootPrivate @ %p\n", _APICID, __KernBootPrivate);
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
        UBYTE apictotal;
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
	apicready = 1;

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

	/* Make kickstart code area read-only */
        core_ProtKernelArea(addr, khi - addr, 1, 0, 1);

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

	/* Explore memory list and create MemHeaders */
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

	/*
	 * Now we have working exec.library memory allocator.
	 * Move console mirror buffer away from unused memory.
	 */
	if (scr_Type == SCR_GFX)
	{
	    char *mirror = AllocMem(scr_Width * scr_Height, MEMF_PUBLIC);

	    fb_SetMirror(mirror);
	}

	D(bug("[Kernel] Created SysBase at 0x%p, MemHeader 0x%p\n", SysBase, mh));

	/* Store important private data */
	__KernBootPrivate->kbp_LowMem = mh;
	TLS_SET(SysBase, SysBase);

	/*
    	 * At this point SysBase is put into 8ULL by PrepareExecBase().
    	 * Store bogus pointer into 0ULL and set the whole page to read-only.
    	 */
	*((IPTR *)0ULL) = 0xC0DEBAD1C0DEBAD2;
	core_ProtKernelArea(0, PAGE_SIZE, 1, 0, 1);

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
	    "pushq $0x3002\n\t"         // rFLANGS
	    "pushq %[cs]\n\t"		// CS
	    "pushq $1f\n\t"
	    "iretq\n 1:"
	    ::[user_ds]"r"(USER_DS),[ds]"i"(USER_DS),[cs]"i"(USER_CS):"r12");

	D(bug("[Kernel] Done?! Still here?\n"));

	apictotal = core_APICGetTotal();
	D(bug("[Kernel] %u APICs detected\n"));
	
	if (apictotal > 1)
    	{
	    bug("[Kernel] Waiting for %d APICs to initialise ..\n", apictotal - 1);
	    while (apicready < apictotal)
	    {
	    	DB2(bug("[Kernel] %d of %d APICs Ready ..\n", apicready, apictotal));
	    }
    	}

	InitCode(RTF_SINGLETASK, 0);
	InitCode(RTF_COLDSTART, 0);

	bug("[Kernel] ERROR: System Boot Failed!\n");
	panic();
    }
    else
    {
	/*
	 * This is a secondary CPU. Currently it has nothing to do :-(
	 */
        UBYTE _APICNO   = core_APICGetNumber();
        UBYTE apictotal = core_APICGetTotal();

        apicready += 1;
        bug("[Kernel] APIC No. %d of %d Going IDLE (Halting)...\n", _APICNO, apictotal);
    }

    while (1) asm volatile("hlt");
}

/* Small delay routine used by exec_cinit initializer */
asm("\ndelay:\t.short   0x00eb\n\tretq");

static uint64_t __attribute__((used, section(".data"), aligned(16))) tmp_stack[128];
static const uint64_t *tmp_stack_end __attribute__((used, section(".text"))) = &tmp_stack[120];

static uint64_t stack[STACK_SIZE] __attribute__((used));
static uint64_t stack_panic[STACK_SIZE] __attribute__((used));
static uint64_t stack_super[STACK_SIZE] __attribute__((used));
static uint64_t stack_ring1[STACK_SIZE] __attribute__((used));

static const uint64_t *stack_end __attribute__((used, section(".text"))) = &stack[STACK_SIZE-16];
static const void *target_address __attribute__((section(".text"),used)) = (void*)kernel_cstart;

static struct int_gate_64bit IGATES[256] __attribute__((used,aligned(256)));
static struct tss_64bit TSS[16] __attribute__((used,aligned(128)));
static struct {
    struct segment_desc seg0;      /* seg 0x00 */
    struct segment_desc super_cs;  /* seg 0x08 */
    struct segment_desc super_ds;  /* seg 0x10 */
    struct segment_desc user_cs32; /* seg 0x18 */
    struct segment_desc user_ds;   /* seg 0x20 */
    struct segment_desc user_cs;   /* seg 0x28 */
    struct segment_desc gs;        /* seg 0x30 */
    struct segment_desc ldt;       /* seg 0x38 */
    struct {
        struct segment_desc tss_low;   /* seg 0x40... */
        struct segment_ext  tss_high;
    } tss[16];        
} GDT __attribute__((used,aligned(128)));

const struct
{
    uint16_t size __attribute__((packed));
    uint64_t base __attribute__((packed));
} 
GDT_sel = {sizeof(GDT)-1, (uint64_t)&GDT};

static tls_t system_tls;

void core_SetupGDT(struct KernBootPrivate *__KernBootPrivate)
{
    IPTR        _APICBase;
    UBYTE       _APICID;
    int i;

    _APICBase = AROS_UFC0(IPTR,
        (*(__KernBootPrivate->kbp_APIC_Drivers[__KernBootPrivate->kbp_APIC_DriverID])->getbase));

    _APICID = (UBYTE)AROS_UFC1(IPTR,
        (*(__KernBootPrivate->kbp_APIC_Drivers[__KernBootPrivate->kbp_APIC_DriverID])->getid),
                AROS_UFCA(IPTR, _APICBase, A0));

    D(rkprintf("[Kernel] core_SetupGDT(%d)\n", _APICID));

    if (_APICID == __KernBootPrivate->kbp_APIC_BSPID)
    {
        /* Supervisor segments */
        GDT.super_cs.type=0x1a;     /* code segment */
        GDT.super_cs.dpl=0;         /* supervisor level */
        GDT.super_cs.p=1;           /* present */
        GDT.super_cs.l=1;           /* long (64-bit) one */
        GDT.super_cs.d=0;           /* must be zero */
        GDT.super_cs.limit_low=0xffff;
        GDT.super_cs.limit_high=0xf;
        GDT.super_cs.g=1;

        GDT.super_ds.type=0x12;     /* data segment */
        GDT.super_ds.dpl=0;         /* supervisor level */
        GDT.super_ds.p=1;           /* present */
        GDT.super_ds.limit_low=0xffff;
        GDT.super_ds.limit_high=0xf;
        GDT.super_ds.g=1;
        GDT.super_ds.d=1;

        /* User mode segments */
        GDT.user_cs.type=0x1a;      /* code segment */
        GDT.user_cs.dpl=3;          /* User level */
        GDT.user_cs.p=1;            /* present */
        GDT.user_cs.l=1;            /* long mode */
        GDT.user_cs.d=0;            /* must be zero */
        GDT.user_cs.limit_low=0xffff;
        GDT.user_cs.limit_high=0xf;
        GDT.user_cs.g=1;

        GDT.user_cs32.type=0x1a;    /* code segment for legacy 32-bit code. NOT USED YET! */
        GDT.user_cs32.dpl=3;        /* user elvel */
        GDT.user_cs32.p=1;          /* present */
        GDT.user_cs32.l=0;          /* 32-bit mode */
        GDT.user_cs32.d=1;          /* 32-bit code */
        GDT.user_cs32.limit_low=0xffff;
        GDT.user_cs32.limit_high=0xf;
        GDT.user_cs32.g=1;

        GDT.user_ds.type=0x12;      /* data segment */
        GDT.user_ds.dpl=3;    /* user elvel */
        GDT.user_ds.p=1;            /* present */
        GDT.user_ds.limit_low=0xffff;
        GDT.user_ds.limit_high=0xf;
        GDT.user_ds.g=1;
        GDT.user_ds.d=1;

        for (i=0; i < 16; i++)
        {
            /* Task State Segment */
            GDT.tss[i].tss_low.type=0x09;      /* 64-bit TSS */
            GDT.tss[i].tss_low.limit_low=sizeof(TSS)-1;
            GDT.tss[i].tss_low.base_low=((unsigned long)&TSS[i]) & 0xffff;
            GDT.tss[i].tss_low.base_mid=(((unsigned long)&TSS[i]) >> 16) & 0xff;
            GDT.tss[i].tss_low.dpl=3;          /* User mode task */
            GDT.tss[i].tss_low.p=1;            /* present */
            GDT.tss[i].tss_low.limit_high=((sizeof(TSS)-1) >> 16) & 0x0f;
            GDT.tss[i].tss_low.base_high=(((unsigned long)&TSS[i]) >> 24) & 0xff;
            GDT.tss[i].tss_high.base_ext = 0;  /* is within 4GB :-D */
        }
        intptr_t tls_ptr = (intptr_t)&system_tls;

        GDT.gs.type=0x12;      /* data segment */
        GDT.gs.dpl=3;    /* user elvel */
        GDT.gs.p=1;            /* present */
        GDT.gs.base_low = tls_ptr & 0xffff;
        GDT.gs.base_mid = (tls_ptr >> 16) & 0xff;
        GDT.gs.base_high = (tls_ptr >> 24) & 0xff;   
        GDT.gs.g=1;
        GDT.gs.d=1;
    }
}

void core_CPUSetup(IPTR _APICBase)
{
    UBYTE _APICID = AROS_UFC1(UBYTE,
                ((struct GenericAPIC *)__KernBootPrivate->kbp_APIC_Drivers[__KernBootPrivate->kbp_APIC_DriverID])->getid,
                        AROS_UFCA(IPTR, _APICBase, A0));

    D(bug("[Kernel] core_CPUSetup(%d)\n", _APICID));
    
    TSS[_APICID].ist1 = (uint64_t)&stack_panic[STACK_SIZE-2];
    TSS[_APICID].rsp0 = (uint64_t)&stack_super[STACK_SIZE-2];
    TSS[_APICID].rsp1 = (uint64_t)&stack_ring1[STACK_SIZE-2];

    bug("[Kernel] core_CPUSetup[%d]: Reloading the GDT and Task Register\n", _APICID);
    asm volatile ("lgdt %0"::"m"(GDT_sel));
    asm volatile ("ltr %w0"::"r"(TASK_SEG + (_APICID << 4)));
    asm volatile ("mov %0,%%gs"::"a"(SEG_GS));    
}

AROS_LH0I(struct TagItem *, KrnGetBootInfo,
         struct KernelBase *, KernelBase, 10, Kernel)
{
    AROS_LIBFUNC_INIT

    return BootMsg;
    
    AROS_LIBFUNC_EXIT
}
