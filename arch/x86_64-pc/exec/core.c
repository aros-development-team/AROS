#include <inttypes.h>
#include <asm/cpu.h>
#include <asm/segments.h>
#include <exec/resident.h>
#include <stdio.h>
#include "core.h"

const struct Resident Core_resident =
{
        RTC_MATCHWORD,          /* Magic value used to find resident */
        &Core_resident,         /* Points to Resident itself */
        &Core_resident+1,       /* Where could we find next Resident? */
        0,                      /* There are no flags!! */
        1,                      /* Version */
        NT_RESOURCE,            /* Type */
        127,                    /* Very high startup priority. */
        (STRPTR)"core.resource",/* Pointer to name string */
        (STRPTR)"$VER: core 1.0 (19.08.2007)\r\n",  /* Ditto */
        NULL                    /* Library initializer (for exec this value is irrelevant since we've jumped there at the begining to bring the system up */
};

int exec_main(struct TagItem *msg, void *entry);

static uint64_t __attribute__((used)) tmp_stack[128]={01,};
static const uint64_t *tmp_stack_end __attribute__((used, section(".text"))) = &tmp_stack[120];

#define STACK_SIZE 8192
static uint64_t stack[STACK_SIZE] __attribute__((used));
static uint64_t stack_panic[STACK_SIZE] __attribute__((used));
static uint64_t stack_super[STACK_SIZE] __attribute__((used));
static const uint64_t *stack_end __attribute__((used, section(".text"))) = &stack[STACK_SIZE-16];

static const void *target_address __attribute__((section(".text"),used)) = (void*)exec_main;

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
GDT_sel = {sizeof(GDT)-1, (uint64_t)&GDT},
IDT_sel = {sizeof(IGATES)-1, (uint64_t)IGATES};

void scr_RawPutChars(char *, int);
void clr();
char tab[512];
#ifdef rkprintf
#undef rkprintf
#endif
#define rkprintf(x...) scr_RawPutChars(tab, snprintf(tab, 510, x))
    
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

asm(".text\n\t"
    ".globl core_DefaultIRETQ\n\t"
    ".type core_DefaultIRETQ,@function\n"
"core_DefaultIRETQ: iretq");



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
    
    TSS.ist1 = (uint64_t)&stack_panic[STACK_SIZE-2];
    TSS.rsp0 = (uint64_t)&stack_super[STACK_SIZE-2];
    
    rkprintf("Reloading the GDT and the Task Register\n");
    asm volatile ("lgdt %0"::"m"(GDT_sel));
    asm volatile ("ltr %w0"::"r"(TASK_SEG));
}

#define STR_(x) #x
#define STR(x) STR_(x)

#define IRQ_NAME_(nr) nr##_intr(void)
#define IRQ_NAME(nr) IRQ_NAME_(IRQ##nr)

#define BUILD_IRQ(nr) \
    void IRQ_NAME(nr); \
asm(".balign 8  ,0x90\n\t" \
    ".globl IRQ" STR(nr) "_intr\n\t" \
    ".type IRQ" STR(nr) "_intr,@function\n" \
    "IRQ" STR(nr) "_intr: pushq $0; pushq $" #nr "\n\t" \
    "jmp core_EnterInterrupt\n\t" \
    ".size IRQ" STR(nr) "_intr, . - IRQ" STR(nr) "_intr" \
);

#define BUILD_IRQ_ERR(nr) \
    void IRQ_NAME(nr); \
asm(".balign 8  ,0x90\n\t" \
    ".globl IRQ" STR(nr) "_intr\n\t" \
    ".type IRQ" STR(nr) "_intr,@function\n" \
    "IRQ" STR(nr) "_intr: pushq $" #nr "\n\t" \
    "jmp core_EnterInterrupt\n\t" \
    ".size IRQ" STR(nr) "_intr, . - IRQ" STR(nr) "_intr" \
);

BUILD_IRQ(0x00)         // Divide-By-Zero Exception
BUILD_IRQ(0x01)         // Debug Exception
BUILD_IRQ(0x02)         // NMI Exception
BUILD_IRQ(0x03)         // Breakpoint Exception
BUILD_IRQ(0x04)         // Overflow Exception
BUILD_IRQ(0x05)         // Bound-Range Exception
BUILD_IRQ(0x06)         // Invalid-Opcode Exception
BUILD_IRQ(0x07)         // Device-Not-Available Exception
BUILD_IRQ_ERR(0x08)     // Double-Fault Exception
BUILD_IRQ(0x09)         // Unused (used to be Coprocesor-Segment-Overrun)
BUILD_IRQ_ERR(0x0a)     // Invalid-TSS Exception
BUILD_IRQ_ERR(0x0b)     // Segment-Not-Present Exception
BUILD_IRQ_ERR(0x0c)     // Stack Exception
BUILD_IRQ_ERR(0x0d)     // General-Protection Exception
BUILD_IRQ_ERR(0x0e)     // Page-Fault Exception
BUILD_IRQ(0x0f)         // Reserved
BUILD_IRQ(0x10)         // Floating-Point Exception
BUILD_IRQ_ERR(0x11)     // Alignment-Check Exception
BUILD_IRQ(0x12)         // Machine-Check Exception
BUILD_IRQ(0x13)         // SIMD-Floating-Point Exception
BUILD_IRQ(0x14) BUILD_IRQ(0x15) BUILD_IRQ(0x16) BUILD_IRQ(0x17) 
BUILD_IRQ(0x18) BUILD_IRQ(0x19) BUILD_IRQ(0x1a) BUILD_IRQ(0x1b)
BUILD_IRQ(0x1c) BUILD_IRQ(0x1d) BUILD_IRQ(0x1e) BUILD_IRQ(0x1f)

#define B(x,y) BUILD_IRQ(x##y)
#define B16(x) \
    B(x,0) B(x,1) B(x,2) B(x,3) B(x,4) B(x,5) B(x,6) B(x,7) \
    B(x,8) B(x,9) B(x,a) B(x,b) B(x,c) B(x,d) B(x,e) B(x,f)

B16(0x2)
BUILD_IRQ(0x80)

void core_IRQHandle(regs_t regs)
{
    rkprintf("IRQ %02x:", regs.irq_number);
    rkprintf("  stack=%04x:%012x rflags=%016x ip=%04x:%012x err=%08x\n",
             regs.return_ss, regs.return_rsp, regs.return_rflags, 
             regs.return_cs, regs.return_rip, regs.error_code);
    
    if (regs.irq_number == 0x20)
        asm ("int $0x21");
}

#define IRQ(x,y) \
        (const void (*)(void))IRQ##x##y##_intr

#define IRQLIST_16(x) \
        IRQ(x,0), IRQ(x,1), IRQ(x,2), IRQ(x,3), \
        IRQ(x,4), IRQ(x,5), IRQ(x,6), IRQ(x,7), \
        IRQ(x,8), IRQ(x,9), IRQ(x,a), IRQ(x,b), \
        IRQ(x,c), IRQ(x,d), IRQ(x,e), IRQ(x,f)

const void __attribute__((section(".text"))) (*interrupt[256])(void) = {
        IRQLIST_16(0x0), IRQLIST_16(0x1), IRQLIST_16(0x2)
};


void core_SetupIDT()
{
    int i;
    uintptr_t off;
    rkprintf("Setting all interrupt handlers to default value\n");
    
    for (i=0; i < 256; i++)
    {
        if (interrupt[i])
            off = (uintptr_t)interrupt[i];
        else if (i == 0x80)
            off = (uintptr_t)&IRQ0x80_intr;
        else
            off = (uintptr_t)&core_DefaultIRETQ;
        
        IGATES[i].offset_low = off & 0xffff;
        IGATES[i].offset_mid = (off >> 16) & 0xffff;
        IGATES[i].offset_high = (off >> 32) & 0xffffffff;
        IGATES[i].type = 0x0e;
        IGATES[i].dpl = 3;
        IGATES[i].p = 1;
        IGATES[i].selector = KERNEL_CS;
        IGATES[i].ist = 0;
    }
    
    asm volatile ("lidt %0"::"m"(IDT_sel));
}
