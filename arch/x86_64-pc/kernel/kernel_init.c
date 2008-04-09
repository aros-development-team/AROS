#define DEBUG 1
#include <aros/debug.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <asm/segments.h>
#include <inttypes.h>
#include <aros/symbolsets.h>
#include <exec/lists.h>

#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include <stdio.h>

#include "kernel_intern.h"
#include "../bootstrap/multiboot.h"
#include LC_LIBDEFS_FILE

#define CONFIG_LAPICS

extern const unsigned char start64[];
extern const unsigned char _binary_smpbootstrap_start[];
extern const unsigned char _binary_smpbootstrap_size[];

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

void __clear_bss(struct TagItem *msg)
{
    struct KernelBSS *bss;
    bss = krnGetTagData(KRN_KernelBss, 0, msg);
    
    if (bss)
    {
        while (bss->addr)
        {
            bzero(bss->addr, bss->len);
            bss++;
        }   
    }
}

/* Post exec init */

IPTR           _kern_initflags;

#define        KERNBOOTFLAG_BOOTCPUSET                1

IPTR           _kern_early_ACPIRSDP;
UBYTE          _kern_early_BOOTAPICID;
IPTR           _Kern_APICTrampolineBase;

static char _kern_early_BOOTCmdLine[200];

static int Kernel_Init(LIBBASETYPEPTR LIBBASE)
{
    int i;
    TLS_SET(KernelBase, LIBBASE);
    struct ExecBase *SysBase = TLS_GET(SysBase);
    
    LIBBASE->kb_XTPIC_Mask = 0xfffb;
     
    for (i=0; i < 256; i++)
    {
        NEWLIST(&LIBBASE->kb_Intr[i]);
        switch(i)
        {
            case 0x20 ... 0x2f:
                LIBBASE->kb_Intr[i].lh_Type = KBL_XTPIC;
                break;
            case 0xfe:
                LIBBASE->kb_Intr[i].lh_Type = KBL_APIC;
                break;
            default:
                LIBBASE->kb_Intr[i].lh_Type = KBL_INTERNAL;
                break;
        }
    }

    D(bug("[Kernel] Kernel_Init: Post-exec init\n"));

    if (LIBBASE->kb_APICBase == NULL)
        LIBBASE->kb_APICBase= core_APICGetMSRAPICBase();;   
    D(bug("[Kernel] Kernel_Init: APIC Base @ %012p\n", LIBBASE->kb_APICBase));

    core_APICInitialise(LIBBASE->kb_APICBase);
    
#warning "TODO: Check if NOACPI is set on the boot command line"
    if (_kern_early_ACPIRSDP)
    {
        LIBBASE->kb_ACPIRSDP = _kern_early_ACPIRSDP;
        LIBBASE->kb_APICCount = 1;
        LIBBASE->kb_APICIDMap = AllocVec(LIBBASE->kb_APICCount, MEMF_CLEAR);
        LIBBASE->kb_APICIDMap[0] = _kern_early_BOOTAPICID;

        core_ACPIInitialise();
    }

    uint32_t *localAPIC = (uint32_t*)LIBBASE->kb_APICBase + 0x320;
    
    LIBBASE->kb_MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC, 8192, 4096);
    D(bug("[Kernel] Kernel_Init: MemPool @ %012p\n", LIBBASE->kb_MemPool));

/*
    asm volatile ("movl %0,(%1)"::"r"(0),"r"((volatile uint32_t*)(LIBBASE->kb_APICBase + 0xb0)));
    
    D(bug("[Kernel] Kernel_Init: APIC SVR=%08x\n", *(volatile uint32_t*)(LIBBASE->kb_APICBase + 0xf0)));
    D(bug("[Kernel] Kernel_Init: APIC ESR=%08x\n", *(volatile uint32_t*)(LIBBASE->kb_APICBase + 0x280)));
    D(bug("[Kernel] Kernel_Init: APIC TPR=%08x\n", *(volatile uint32_t*)(LIBBASE->kb_APICBase + 0x80)));
    D(bug("[Kernel] Kernel_Init: APIC ICR=%08x%08x\n", *(volatile uint32_t*)(LIBBASE->kb_APICBase + 0x314), *(volatile uint32_t*)(LIBBASE->kb_APICBase + 0x310)));
    D(bug("[Kernel] Kernel_Init: APIC Timer divide=%08x\n", *(volatile uint32_t*)(LIBBASE->kb_APICBase + 0x3e0)));
    D(bug("[Kernel] Kernel_Init: APIC Timer config=%08x\n", *(volatile uint32_t*)(LIBBASE->kb_APICBase + 0x320)));
    
    asm volatile ("movl %0,(%1)"::"r"(0x000000fe),"r"((volatile uint32_t*)(LIBBASE->kb_APICBase + 0x320)));
    //*(volatile uint32_t *)localAPIC = 0x000000fe;
    D(bug("[Kernel] Kernel_Init: APIC Timer config=%08x\n", *(volatile uint32_t*)(LIBBASE->kb_APICBase + 0x320)));
    
    D(bug("[Kernel] Kernel_Init: APIC Initial count=%08x\n", *(volatile uint32_t*)(LIBBASE->kb_APICBase + 0x380)));
    D(bug("[Kernel] Kernel_Init: APIC Current count=%08x\n", *(volatile uint32_t*)(LIBBASE->kb_APICBase + 0x390)));
    *(volatile uint32_t*)(LIBBASE->kb_APICBase + 0x380) = 0x11111111;
    asm volatile ("movl %0,(%1)"::"r"(0x000200fe),"r"((volatile uint32_t*)(LIBBASE->kb_APICBase + 0x320)));
    D(bug("[Kernel] Kernel_Init: APIC Timer config=%08x\n", *(volatile uint32_t*)(LIBBASE->kb_APICBase + 0x320)));
    
    for (i=0; i < 0x10000000; i++) asm volatile("nop;");
    
    D(bug("[Kernel] Kernel_Init: APIC Initial count=%08x\n", *(volatile uint32_t*)(LIBBASE->kb_APICBase + 0x380)));
    D(bug("[Kernel] Kernel_Init: APIC Current count=%08x\n", *(volatile uint32_t*)(LIBBASE->kb_APICBase + 0x390)));
    for (i=0; i < 0x1000000; i++) asm volatile("nop;");
    D(bug("[Kernel] Kernel_Init: APIC Initial count=%08x\n", *(volatile uint32_t*)(LIBBASE->kb_APICBase + 0x380)));
    D(bug("[Kernel] Kernel_Init: APIC Current count=%08x\n", *(volatile uint32_t*)(LIBBASE->kb_APICBase + 0x390)));

    for (i=0; i < 0x1000000; i++) asm volatile("nop;"); */
}

ADD2INITLIB(Kernel_Init, 0)

static struct TagItem *BootMsg;
static struct vbe_controller vbectrl;
static struct vbe_mode vbemd;
intptr_t addr;
intptr_t len;

int kernel_cstart(struct TagItem *msg, void *entry)
{
    rkprintf("[Kernel] kernel_cstart: Jumped into kernel.resource @ %p [asm stub @ %p].\n", kernel_cstart, start64);

    IPTR _APICBase = core_APICGetMSRAPICBase();
    UBYTE kern_apic_id = core_APICGetID(_APICBase);
    rkprintf("[Kernel] kernel_cstart: launching on APIC ID %d, base @ %p\n", kern_apic_id, _APICBase);

    /* Enable fxsave/fxrstor */ 
    wrcr(cr4, rdcr(cr4) | _CR4_OSFXSR | _CR4_OSXMMEXCPT);
    
    if (!(_kern_initflags & KERNBOOTFLAG_BOOTCPUSET))
    {
        _kern_early_BOOTAPICID = kern_apic_id;
        _kern_initflags |= KERNBOOTFLAG_BOOTCPUSET;

        struct TagItem *tag = krnFindTagItem(KRN_CmdLine, msg);
        addr = krnGetTagData(KRN_KernelBase, 0, msg);
        len = krnGetTagData(KRN_KernelHighest, 0, msg) - addr;

        if (tag)
        {
            if (tag->ti_Data != (IPTR)_kern_early_BOOTCmdLine) {
                strncpy(_kern_early_BOOTCmdLine, tag->ti_Data, 200);
                tag->ti_Data = (IPTR)_kern_early_BOOTCmdLine;
            }
        }

        tag = krnFindTagItem(KRN_VBEModeInfo, msg);
        if (tag)
        {
            if (tag->ti_Data != (IPTR)&vbemd) {
                bcopy(tag->ti_Data, &vbemd, sizeof(vbemd));
                tag->ti_Data = (IPTR)&vbemd;
            }
        }

        tag = krnFindTagItem(KRN_VBEControllerInfo, msg);
        if (tag)
        {
            if (tag->ti_Data != (IPTR)&vbectrl) {
                bcopy(tag->ti_Data, &vbectrl, sizeof(vbectrl));
                tag->ti_Data = (IPTR)&vbectrl;
            }
        }

        BootMsg = msg;

        core_APICProbe();

        /* Initialize the ACPI boot-time table parser. */
        _kern_early_ACPIRSDP = core_ACPIProbe();
        rkprintf("[Kernel] kernel_cstart: core_ACPIProbe() returned %p\n", _kern_early_ACPIRSDP);

#warning "TODO: Allocate Trampoline page better"

        IPTR lowpages = (krnGetTagData(KRN_MEMLower, 0, msg) * 1024);
        if ((lowpages > 0x2000) && ((lowpages - 0x2000) > PAGE_SIZE))
        {
            _Kern_APICTrampolineBase = (lowpages - PAGE_SIZE) & 0xFF000;
            lowpages = (_Kern_APICTrampolineBase - 1)/1024;

            krnSetTagData(KRN_MEMLower, lowpages, msg);
            rkprintf("[Kernel] kernel_cstart: Allocated %d bytes for APIC Trampoline @ %p\n", PAGE_SIZE, _Kern_APICTrampolineBase);

#if defined(CONFIG_LAPICS)       
            memcpy(_Kern_APICTrampolineBase, _binary_smpbootstrap_start,
                        _binary_smpbootstrap_size);

            rkprintf("[Kernel] kernel_cstart: Copied APIC bootstrap code to Trampoline from %p, %d bytes\n", _binary_smpbootstrap_start,
                        _binary_smpbootstrap_size);
#endif
        }

        /* Prepair GDT */
        core_SetupGDT();

        /* Set TSS, GDT, LDT and MMU up */
        core_CPUSetup(_APICBase);
        core_SetupIDT();
        core_SetupMMU();

        core_ProtKernelArea(addr, len, 1, 0, 1);

        /* Lock page 0! */
        core_ProtKernelArea(0, 1, 0, 0, 0);
    }
    else
    {
        core_CPUSetup(_APICBase);
        core_SetupIDT();
    }

    (rkprintf("[Kernel] kernel_cstart[%d]: APIC_BASE_MSR=%016p\n", kern_apic_id, rdmsrq(27)));

    if (kern_apic_id == _kern_early_BOOTAPICID)
    {
        /* Setup the 8259 */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x11),"i"(0x20)); /* Initialization sequence for 8259A-1 */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x11),"i"(0xa0)); /* Initialization sequence for 8259A-2 */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x20),"i"(0x21)); /* IRQs at 0x20 - 0x27 */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x28),"i"(0xa1)); /* IRQs at 0x28 - 0x2f */
        asm("outb   %b0,%b1\n\tcall delay": :"a"((char)0x04),"i"(0x21)); /* 8259A-1 is master */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x02),"i"(0xa1)); /* 8259A-2 is slave */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x01),"i"(0x21)); /* 8086 mode for both */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x01),"i"(0xa1));
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0xff),"i"(0x21)); /* Enable cascade int */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0xff),"i"(0xa1)); /* Mask all interrupts */

        rkprintf("[Kernel] kernel_cstart: Interrupts redirected. We will go back in a minute ;)\n");
        rkprintf("[Kernel] kernel_cstart: Booting exec.library\n\n");

        return exec_main(msg, entry);
    }

//    else
    {
        /* A temporary solution - the code for smp is not ready yet... */
#warning "TODO: launch idle task ..."
        rkprintf("[Kernel] kernel_cstart[%d]: Going into endless loop...\n", kern_apic_id);
        while(1) asm volatile("hlt");
    }

    return NULL;    
}


/* Small delay routine used by exec_cinit initializer */
asm("\ndelay:\t.short   0x00eb\n\tretq");

static uint64_t __attribute__((used)) tmp_stack[128]={01,};
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

void core_SetupGDT()
{
    int i;
    
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
        GDT.tss[i].tss_low.base_low=((unsigned int)&TSS[i]) & 0xffff;
        GDT.tss[i].tss_low.base_mid=(((unsigned int)&TSS[i]) >> 16) & 0xff;
        GDT.tss[i].tss_low.dpl=3;          /* User mode task */
        GDT.tss[i].tss_low.p=1;            /* present */
        GDT.tss[i].tss_low.limit_high=((sizeof(TSS)-1) >> 16) & 0x0f;
        GDT.tss[i].tss_low.base_high=(((unsigned int)&TSS[i]) >> 24) & 0xff;
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


void core_CPUSetup(IPTR _APICBase)
{
    UBYTE CPU_ID = core_APICGetID(_APICBase);
    rkprintf("[Kernel] core_CPUSetup(id:%d)\n", CPU_ID);
    
//    system_tls.SysBase = (struct ExecBase *)0x12345678;
    
    TSS[CPU_ID].ist1 = (uint64_t)&stack_panic[STACK_SIZE-2];
    TSS[CPU_ID].rsp0 = (uint64_t)&stack_super[STACK_SIZE-2];
    TSS[CPU_ID].rsp1 = (uint64_t)&stack_ring1[STACK_SIZE-2];

    rkprintf("[Kernel] core_CPUSetup[%d]: Reloading the GDT and Task Register\n", CPU_ID);
    asm volatile ("lgdt %0"::"m"(GDT_sel));
    asm volatile ("ltr %w0"::"r"(TASK_SEG + (CPU_ID << 4)));
    asm volatile ("mov %0,%%gs"::"a"(SEG_GS));    
}

struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr)
{
    if (!(*tagListPtr)) return 0;

    while(1)
    {
        switch((*tagListPtr)->ti_Tag)
        {
            case TAG_MORE:
                if (!((*tagListPtr) = (struct TagItem *)(*tagListPtr)->ti_Data))
                    return NULL;
                continue;
            case TAG_IGNORE:
                break;

            case TAG_END:
                (*tagListPtr) = 0;
                return NULL;

            case TAG_SKIP:
                (*tagListPtr) += (*tagListPtr)->ti_Data + 1;
                continue;

            default:
                return (struct TagItem *)(*tagListPtr)++;

        }

        (*tagListPtr)++;
    }
}

struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList)
{
    struct TagItem *tag;
    const struct TagItem *tagptr = tagList;

    while((tag = krnNextTagItem(&tagptr)))
    {
        if (tag->ti_Tag == tagValue)
            return tag;
    }

    return 0;
}

IPTR krnGetTagData(Tag tagValue, intptr_t defaultVal, const struct TagItem *tagList)
{
    struct TagItem *ti = 0;

    if (tagList && (ti = krnFindTagItem(tagValue, tagList)))
        return ti->ti_Data;

        return defaultVal;
}

void krnSetTagData(Tag tagValue, intptr_t newtagValue, const struct TagItem *tagList)
{
    struct TagItem *ti = 0;

    if (tagList && (ti = krnFindTagItem(tagValue, tagList)))
        ti->ti_Data = newtagValue;
}

AROS_LH0I(struct TagItem *, KrnGetBootInfo,
         struct KernelBase *, KernelBase, 10, Kernel)
{
    AROS_LIBFUNC_INIT

    return BootMsg;
    
    AROS_LIBFUNC_EXIT
}

