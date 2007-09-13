#define DEBUG 1
#include <aros/debug.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <asm/segments.h>
#include <inttypes.h>
#include <aros/symbolsets.h>
#include <exec/lists.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include <stdio.h>

#include "kernel_intern.h"
#include LC_LIBDEFS_FILE

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

static int Kernel_Init(LIBBASETYPEPTR LIBBASE)
{
    int i;
    TLS_SET(KernelBase, LIBBASE);
    struct ExecBase *SysBase = TLS_GET(SysBase);
    
    LIBBASE->kb_XTPIC_Mask = 0xffff;
     
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
     
    
    uint32_t *localAPIC = (uint32_t*)0xfee00320;

    D(bug("[Kernel] Post-exec init\n"));
    
    LIBBASE->kb_MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC, 8192, 4096);
    D(bug("[Kernel] MemPool=%012p\n", LIBBASE->kb_MemPool));
/*    
    asm volatile ("movl %0,(%1)"::"r"(0),"r"(0xfee000b0));
    
    D(bug("[Kernel] APIC SVR=%08x\n", *(uint32_t*)0xfee000f0));
    D(bug("[Kernel] APIC ESR=%08x\n", *(uint32_t*)0xfee00280));
    D(bug("[Kernel] APIC TPR=%08x\n", *(uint32_t*)0xfee00080));
    D(bug("[Kernel] APIC ICR=%08x%08x\n", *(uint32_t*)0xfee00314, *(uint32_t*)0xfee00310));
    D(bug("[Kernel] APIC Timer divide=%08x\n", *(uint32_t*)0xfee003e0));
    D(bug("[Kernel] APIC Timer config=%08x\n", *(uint32_t*)0xfee00320));
    
    asm volatile ("movl %0,(%1)"::"r"(0x000000fe),"r"(0xfee00320));
    //*(volatile uint32_t *)localAPIC = 0x000000fe;
    D(bug("[Kernel] APIC Timer config=%08x\n", *(uint32_t*)0xfee00320));
    
    D(bug("[Kernel] APIC Initial count=%08x\n", *(uint32_t*)0xfee00380));
    D(bug("[Kernel] APIC Current count=%08x\n", *(uint32_t*)0xfee00390));
    *(uint32_t*)0xfee00380 = 0x11111111;
    asm volatile ("movl %0,(%1)"::"r"(0x000200fe),"r"(0xfee00320));
    D(bug("[Kernel] APIC Timer config=%08x\n", *(uint32_t*)0xfee00320));
    
    for (i=0; i < 0x10000000; i++) asm volatile("nop;");
    
    D(bug("[Kernel] APIC Initial count=%08x\n", *(uint32_t*)0xfee00380));
    D(bug("[Kernel] APIC Current count=%08x\n", *(uint32_t*)0xfee00390));
    for (i=0; i < 0x1000000; i++) asm volatile("nop;");
    D(bug("[Kernel] APIC Initial count=%08x\n", *(uint32_t*)0xfee00380));
    D(bug("[Kernel] APIC Current count=%08x\n", *(uint32_t*)0xfee00390));

    for (i=0; i < 0x1000000; i++) asm volatile("nop;"); */
}

ADD2INITLIB(Kernel_Init, 0)

int kernel_cstart(struct TagItem *msg, void *entry)
{
    rkprintf("[Kernel] Booting into kernel.resource...");

    /* Set TSS, GDT, LDT and MMU up */
    core_SetupGDT();
    core_SetupIDT();

    (rkprintf("[Kernel] APIC_BASE_MSR=%016p\n", rdmsrq(27)));

    /* Setu the 8259 up */
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

    rkprintf("[Kernel] Interrupts redirected. We will go back in a minute ;)\n");
    rkprintf("[Kernel] Booting exec.library\n\n");

    return exec_main(msg, entry);
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
static struct tss_64bit TSS __attribute__((used,aligned(128)));
static struct {
    struct segment_desc seg0;      /* seg 0x00 */
    struct segment_desc super_cs;  /* seg 0x08 */
    struct segment_desc super_ds;  /* seg 0x10 */
    struct segment_desc user_cs32; /* seg 0x18 */
    struct segment_desc user_ds;   /* seg 0x20 */
    struct segment_desc user_cs;   /* seg 0x28 */
    struct segment_desc tss_low;   /* seg 0x30 */
    struct segment_ext  tss_high;
    struct segment_desc gs;        /* seg 0x40 */
    struct segment_desc ldt;       /* seg 0x48 */
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

    /* Task State Segment */
    GDT.tss_low.type=0x09;      /* 64-bit TSS */
    GDT.tss_low.limit_low=sizeof(TSS)-1;
    GDT.tss_low.base_low=((unsigned int)&TSS) & 0xffff;
    GDT.tss_low.base_mid=(((unsigned int)&TSS) >> 16) & 0xff;
    GDT.tss_low.dpl=3;          /* User mode task */
    GDT.tss_low.p=1;            /* present */
    GDT.tss_low.limit_high=((sizeof(TSS)-1) >> 16) & 0x0f;
    GDT.tss_low.base_high=(((unsigned int)&TSS) >> 24) & 0xff;
    GDT.tss_high.base_ext = 0;  /* is within 4GB :-D */

    intptr_t tls_ptr = (intptr_t)&system_tls;
    
    GDT.gs.type=0x12;      /* data segment */
    GDT.gs.dpl=3;    /* user elvel */
    GDT.gs.p=1;            /* present */
    GDT.gs.base_low = tls_ptr & 0xffff;
    GDT.gs.base_mid = (tls_ptr >> 16) & 0xff;
    GDT.gs.base_high = (tls_ptr >> 24) & 0xff;   
    GDT.gs.g=1;
    GDT.gs.d=1;

    system_tls.SysBase = (struct ExecBase *)0x12345678;
    
    TSS.ist1 = (uint64_t)&stack_panic[STACK_SIZE-2];
    TSS.rsp0 = (uint64_t)&stack_super[STACK_SIZE-2];
    TSS.rsp1 = (uint64_t)&stack_ring1[STACK_SIZE-2];

    rkprintf("[Kernel] Reloading the GDT and the Task Register\n");
    asm volatile ("lgdt %0"::"m"(GDT_sel));
    asm volatile ("ltr %w0"::"r"(TASK_SEG));
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
