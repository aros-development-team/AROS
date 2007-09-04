#include <inttypes.h>
#include <asm/cpu.h>
#include <asm/segments.h>
#include <aros/asmcall.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <exec/lists.h>
#include <hardware/intbits.h>
#include <stdio.h>
#include <asm/cpu.h>
#include "core.h"
#include "exec_intern.h"
#include "etask.h"

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

static tls_t system_tls;

#define STACK_SIZE 8192
static uint64_t stack[STACK_SIZE] __attribute__((used));
static uint64_t stack_panic[STACK_SIZE] __attribute__((used));
static uint64_t stack_super[STACK_SIZE] __attribute__((used));
static uint64_t stack_ring1[STACK_SIZE] __attribute__((used));
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

    rkprintf("Reloading the GDT and the Task Register\n");
    asm volatile ("lgdt %0"::"m"(GDT_sel));
    asm volatile ("ltr %w0"::"r"(TASK_SEG));
    asm volatile ("mov %0,%%gs"::"a"(SEG_GS));
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


#define SAVE_REGS        \
        "pushq %rax; pushq %rbp; pushq %rbx; pushq %rdi; pushq %rsi; pushq %rdx;"  \
        "pushq %rcx; pushq %r8; pushq %r9; pushq %r10; pushq %r11; pushq %r12;"  \
        "pushq %r13; pushq %r14; pushq %r15; mov %ds,%eax; pushq %rax;"
        
#define RESTORE_REGS    \
        "popq %rax; mov %ax,%ds; mov %ax,%es; popq %r15; popq %r14; popq %r13;"  \
        "popq %r12; popq %r11; popq %r10; popq %r9; popq %r8; popq %rcx;" \
        "popq %rdx; popq %rsi; popq %rdi; popq %rbx; popq %rbp; popq %rax"

asm(
"                .balign 32,0x90      \n"
"                .globl core_EnterInterrupt \n"
"                .type core_EnterInterrupt,@function \n"
"core_EnterInterrupt: \n\t" SAVE_REGS "\n"
"                movl    $" STR(KERNEL_DS) ",%eax \n"
"                mov     %ax,%ds \n"
"                mov     %ax,%es \n"
"                movq    %rsp,%rdi \n"
"                call    core_IRQHandle \n"
"                movq    %rsp,%rdi \n"
"                jmp     core_ExitInterrupt \n"
"                .size core_EnterInterrupt, .-core_EnterInterrupt"      
); 

asm(
"                .balign 32,0x90      \n"
"                .globl core_LeaveInterrupt \n"
"                .type core_LeaveInterrupt,@function \n"
"core_LeaveInterrupt: movq %rdi,%rsp \n\t" RESTORE_REGS "\n"                
"                addq $16,%rsp \n"
"                iretq \n"
"                .size core_LeaveInterrupt, .-core_LeaveInterrupt"
);

void core_foo()
{
    rkprintf("foo\n");
}
void core_bar()
{
    rkprintf("bar\n");
}

void core_Cause(struct ExecBase *SysBase);
void core_LeaveInterrupt(regs_t *regs) __attribute__((noreturn));
void core_Switch(regs_t *regs) __attribute__((noreturn));
void core_Schedule(regs_t *regs) __attribute__((noreturn));
void core_Dispatch(regs_t *regs) __attribute__((noreturn));
void core_ExitInterrupt(regs_t *regs) __attribute__((noreturn)); 
void core_IRQHandle(regs_t regs);

/*
 * Task dispatcher. Basically it may be the same one no matter what scheduling algorithm is used
 */
void core_Dispatch(regs_t *regs)
{
    struct ExecBase *SysBase;
    struct Task *task;
    SysBase = *(struct ExecBase **)4UL;
    
    __asm__ __volatile__("cli;");
    
    /* 
     * Is the list of ready tasks empty? Well, increment the idle switch cound and halt CPU.
     * It should be extended by some plugin mechanism which would put CPU and whole machine
     * into some more sophisticated sleep states (ACPI?)
     */
    while (IsListEmpty(&SysBase->TaskReady))
    {
        SysBase->IdleCount++;
        SysBase->AttnResched |= ARF_AttnSwitch;
        
        /* Sleep almost forever ;) */
        __asm__ __volatile__("sti; hlt; cli");
        
        if (SysBase->SysFlags & SFF_SoftInt)
        {
            core_Cause(SysBase);
        }
    }

    SysBase->DispCount++;
    
    /* Get the first task from the TaskReady list, and populate it's settings through Sysbase */
    task = (struct Task *)REMHEAD(&SysBase->TaskReady);
    SysBase->ThisTask = task;  
    SysBase->Elapsed = SysBase->Quantum;
    SysBase->SysFlags &= ~0x2000;
    task->tc_State = TS_RUN;
    SysBase->IDNestCnt = task->tc_IDNestCnt;
   
    /* Handle tasks's flags */
    if (task->tc_Flags & TF_EXCEPT)
        Exception();
    
    if (task->tc_Flags & TF_LAUNCH)
    {
        AROS_UFC1(void, task->tc_Launch,
                  AROS_UFCA(struct ExecBase *, SysBase, A6));       
    }
    
    /* Restore the task's state */
    bcopy(GetIntETask(task)->iet_Context, regs, sizeof(regs_t));
    /* Copy the fpu, mmx, xmm state */
#warning FIXME: Change to the lazy saving of the XMM state!!!!
    asm volatile("fxrstor (%0)"::"D"((char *)GetIntETask(task)->iet_Context + sizeof(regs_t)));
    
    /* Leave interrupt and jump to the new task */
    core_LeaveInterrupt(regs);
}

void core_Switch(regs_t *regs)
{
    struct ExecBase *SysBase;
    struct Task *task;
    
    /* Disable interrupts for a while */
    __asm__ __volatile__("cli; cld;");
        
    SysBase = *(struct ExecBase **)4UL;
    task = SysBase->ThisTask;
    
    /* Copy current task's context into the ETask structure */
    bcopy(regs, GetIntETask(task)->iet_Context, sizeof(regs_t));
    
    /* Copy the fpu, mmx, xmm state */
#warning FIXME: Change to the lazy saving of the XMM state!!!!
    asm volatile("fxsave (%0)"::"D"((char *)GetIntETask(task)->iet_Context + sizeof(regs_t)));
    
    /* store IDNestCnt into tasks's structure */  
    task->tc_IDNestCnt = SysBase->IDNestCnt;
    task->tc_SPReg = regs->return_rsp;
    
    /* And enable interrupts */
    SysBase->IDNestCnt = -1;
    __asm__ __volatile__("sti;");
    
    /* TF_SWITCH flag set? Call the switch routine */
    if (task->tc_Flags & TF_SWITCH)
    {
        AROS_UFC1(void, task->tc_Switch,
                  AROS_UFCA(struct ExecBase *, SysBase, A6));
    }
    
    core_Dispatch(regs);
}

/*
 * Schedule the currently running task away. Put it into the TaskReady list 
 * in some smart way. This function is subject of change and it will be probably replaced
 * by some plugin system in the future
 */ 
void core_Schedule(regs_t *regs)
{
    struct ExecBase *SysBase;
    struct Task *task;

    /* Disable interrupts for a while */
    __asm__ __volatile__("cli");

    SysBase = *(struct ExecBase **)4UL;
    task = SysBase->ThisTask;

    /* Clear the pending switch flag. */
    SysBase->AttnResched &= ~ARF_AttnSwitch;

    /* If task has pending exception, reschedule it so that the dispatcher may handle the exception */
    if (!(task->tc_Flags & TF_EXCEPT))
    {
        /* Is the TaskReady empty? If yes, then the running task is the only one. Let it work */
        if (IsListEmpty(&SysBase->TaskReady))
            core_LeaveInterrupt(regs);

        /* Does the TaskReady list contains tasks with priority equal or lower than current task?
         * If so, then check further... */
        if (((struct Task*)GetHead(&SysBase->TaskReady))->tc_Node.ln_Pri <= task->tc_Node.ln_Pri)
        {
            /* If the running task did not used it's whole quantum yet, let it work */
            if (!(SysBase->SysFlags & 0x2000))
            {
                core_LeaveInterrupt(regs);
            }
        }
    }

    /* 
     * If we got here, then the rescheduling is necessary. 
     * Put the task into the TaskReady list.
     */
    task->tc_State = TS_READY;
    Enqueue(&SysBase->TaskReady, (struct Node *)task);
    
    /* Select new task to run */
    core_Switch(regs);
}

/*
 * Leave the interrupt. This function recieves the register frame used to leave the supervisor
 * mode. It never returns and reschedules the task if it was asked for.
 */
void core_ExitInterrupt(regs_t *regs) 
{
    struct ExecBase *SysBase;

    /* Going back into supervisor mode? Then exit immediatelly */
    if (regs->ds == KERNEL_DS)
    {
        core_LeaveInterrupt(regs);
    }
    else
    {
        /* Prepare to go back into user mode */
        SysBase = *(struct ExecBase **)4UL;

        /* Soft interrupt requested? It's high time to do it */
        if (SysBase->SysFlags & SFF_SoftInt)
            core_Cause(SysBase);

        /* If task switching is disabled, leave immediatelly */
        if (SysBase->TDNestCnt > 0)
        {
            core_LeaveInterrupt(regs);
        }
        else
        {
            /* 
             * Do not disturb task if it's not necessary. 
             * Reschedule only if switch pending flag is set. Exit otherwise.
             */
            if (SysBase->AttnResched & ARF_AttnSwitch)
            {
                core_Schedule(regs);
            }
            else 
                core_LeaveInterrupt(regs);
        }
    }
}

void core_IRQHandle(regs_t regs)
{
    struct ExecBase *SysBase = *(struct ExecBase **)4;
    
    rkprintf("IRQ %02x:", regs.irq_number);
    rkprintf("  stack=%04x:%012x rflags=%016x ip=%04x:%012x err=%08x\n",
             regs.return_ss, regs.return_rsp, regs.return_rflags, 
             regs.return_cs, regs.return_rip, regs.error_code);

    if (regs.irq_number == 0x20)
        asm ("int $0x21");

    
    if (regs.irq_number == 0x03)        /* Debug */
    {
        rkprintf("  INT3 debug fault!\n");
        rkprintf("  rax=%016lx rbx=%016lx rcx=%016lx rdx=%016lx\n", regs.rax, regs.rbx, regs.rcx, regs.rdx);
        rkprintf("  rsi=%016lx rdi=%016lx rbp=%016lx rsp=%016lx\n", regs.rsi, regs.rdi, regs.rbp, regs.return_rsp);
        rkprintf("  r08=%016lx r09=%016lx r10=%016lx r11=%016lx\n", regs.r8, regs.r9, regs.r10, regs.r11);
        rkprintf("  r12=%016lx r13=%016lx r14=%016lx r15=%016lx\n", regs.r12, regs.r13, regs.r14, regs.r15);
    }
    else if (regs.irq_number == 0x0D)        /* GPF */
    {
        rkprintf("  GENERAL PROTECTION FAULT!\n");
        rkprintf("  rax=%016lx rbx=%016lx rcx=%016lx rdx=%016lx\n", regs.rax, regs.rbx, regs.rcx, regs.rdx);
        rkprintf("  rsi=%016lx rdi=%016lx rbp=%016lx rsp=%016lx\n", regs.rsi, regs.rdi, regs.rbp, regs.return_rsp);
        rkprintf("  r08=%016lx r09=%016lx r10=%016lx r11=%016lx\n", regs.r8, regs.r9, regs.r10, regs.r11);
        rkprintf("  r12=%016lx r13=%016lx r14=%016lx r15=%016lx\n", regs.r12, regs.r13, regs.r14, regs.r15);
        while(1);
    }
    else if (regs.irq_number == 0x0e)        /* Page fault */
    {
        void *ptr = rdcr(cr2);
        
        rkprintf("  PAGE FAULT EXCEPTION! %016p\n",ptr);
        while(1);
    }
    else if (regs.irq_number == 0x80) /* Syscall? */
    {
        switch (regs.rax)
        {
            case SC_CAUSE:
                core_Cause(SysBase);
                break;

            case SC_DISPATCH:
                core_Dispatch(&regs);
                break;
                
            case SC_SWITCH:
                core_Switch(&regs);
                break;
            
            case SC_SCHEDULE:
                if (regs.ds != KERNEL_DS)
                    core_Schedule(&regs);
                break;
        }
    }
}

void core_Cause(struct ExecBase *SysBase)
{
    struct IntVector *iv = &SysBase->IntVects[INTB_SOFTINT];

    /* If the SoftInt vector in SysBase is set, call it. It will do the rest for us */
    if (iv->iv_Code)
    {
        AROS_UFC5(void, iv->iv_Code,
            AROS_UFCA(ULONG, 0, D1),
            AROS_UFCA(ULONG, 0, A0),
            AROS_UFCA(APTR, 0, A1),
            AROS_UFCA(APTR, iv->iv_Code, A5),
            AROS_UFCA(struct ExecBase *, SysBase, A6)
        );
    }
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
