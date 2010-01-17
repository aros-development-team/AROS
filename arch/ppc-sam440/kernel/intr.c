#include <aros/kernel.h>
#include <aros/libcall.h>
#include <asm/amcc440.h>
#include <stddef.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel_intern.h"
#include "syscall.h"

AROS_LH4(void *, KrnAddIRQHandler,
         AROS_LHA(uint8_t, irq, D0),
         AROS_LHA(void *, handler, A0),
         AROS_LHA(void *, handlerData, A1),
         AROS_LHA(void *, handlerData2, A2),
         struct KernelBase *, KernelBase, 7, Kernel)
{
    AROS_LIBFUNC_INIT

    struct ExecBase *SysBase = getSysBase();
    struct IntrNode *handle = NULL;
    D(bug("[KRN] KrnAddIRQHandler(%02x, %012p, %012p, %012p):\n", irq, handler, handlerData, handlerData2));

    if (irq < 63)
    {
        /* Go to supervisor mode */
        goSuper();

        handle = Allocate(KernelBase->kb_SupervisorMem, sizeof(struct IntrNode));
        D(bug("[KRN]   handle=%012p\n", handle));

        if (handle)
        {
            handle->in_Handler = handler;
            handle->in_HandlerData = handlerData;
            handle->in_HandlerData2 = handlerData2;
            handle->in_type = it_interrupt;
            handle->in_nr = irq;

            Disable();
            ADDHEAD(&KernelBase->kb_Interrupts[irq], &handle->in_Node);

            if (irq < 32)
            {
                wrdcr(UIC0_ER, rddcr(UIC0_ER) | (0x80000000 >> irq));
            }
            else
            {
                wrdcr(UIC1_ER, rddcr(UIC1_ER) | (0x80000000 >> (irq - 32)));
                wrdcr(UIC0_ER, rddcr(UIC0_ER) | 0x00000003);
            }

            Enable();
        }

        goUser();
    }

    return handle;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, KrnRemIRQHandler,
         AROS_LHA(void *, handle, A0),
         struct KernelBase *, KernelBase, 8, Kernel)
{
    AROS_LIBFUNC_INIT

    struct ExecBase *SysBase = getSysBase();
    struct IntrNode *h = handle;
    uint8_t irq = h->in_nr;

    if (h && (h->in_type == it_interrupt))
    {
        goSuper();

        Disable();
        REMOVE(h);
        if (IsListEmpty(&KernelBase->kb_Interrupts[irq]))
        {
            if (irq < 30)
            {
                wrdcr(UIC0_ER, rddcr(UIC0_ER) & ~(0x80000000 >> irq));
            }
            else if (irq > 31)
            {
                wrdcr(UIC1_ER, rddcr(UIC0_ER) & ~(0x80000000 >> (irq - 32)));
            }
        }
        Enable();

        Deallocate(KernelBase->kb_SupervisorMem, h, sizeof(struct IntrNode));

        goUser();
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH4(void *, KrnAddExceptionHandler,
         AROS_LHA(uint8_t, irq, D0),
         AROS_LHA(void *, handler, A0),
         AROS_LHA(void *, handlerData, A1),
         AROS_LHA(void *, handlerData2, A2),
         struct KernelBase *, KernelBase, 7, Kernel)
{
    AROS_LIBFUNC_INIT

    struct ExecBase *SysBase = getSysBase();
    struct IntrNode *handle = NULL;
    D(bug("[KRN] KrnAddExceptionHandler(%02x, %012p, %012p, %012p):\n", irq, handler, handlerData, handlerData2));

    if (irq < 16)
    {
        /* Go to supervisor mode */
        goSuper();

        handle = Allocate(KernelBase->kb_SupervisorMem, sizeof(struct IntrNode));
        D(bug("[KRN]   handle=%012p\n", handle));

        if (handle)
        {
            handle->in_Handler = handler;
            handle->in_HandlerData = handlerData;
            handle->in_HandlerData2 = handlerData2;
            handle->in_type = it_exception;
            handle->in_nr = irq;

            Disable();
            ADDHEAD(&KernelBase->kb_Exceptions[irq], &handle->in_Node);
            Enable();
        }

        goUser();
    }

    return handle;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, KrnRemExceptionHandler,
         AROS_LHA(void *, handle, A0),
         struct KernelBase *, KernelBase, 8, Kernel)
{
    AROS_LIBFUNC_INIT

    struct ExecBase *SysBase = getSysBase();
    struct IntrNode *h = handle;

    if (h && (h->in_type == it_exception))
    {
        goSuper();

        Disable();
        REMOVE(h);
        Enable();

        Deallocate(KernelBase->kb_SupervisorMem, h, sizeof(struct IntrNode));

        goUser();
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH0I(void, KrnCli,
         struct KernelBase *, KernelBase, 9, Kernel)
{
    AROS_LIBFUNC_INIT

    asm volatile("li %%r3,%0; sc"::"i"(SC_CLI):"memory","r3");

    AROS_LIBFUNC_EXIT
}

AROS_LH0I(void, KrnSti,
         struct KernelBase *, KernelBase, 10, Kernel)
{
    AROS_LIBFUNC_INIT

    asm volatile("li %%r3,%0; sc"::"i"(SC_STI):"memory","r3");

    AROS_LIBFUNC_EXIT
}

AROS_LH0I(void, KrnIsSuper,
         struct KernelBase *, KernelBase, 12, Kernel)
{
    AROS_LIBFUNC_INIT

    register int retval asm ("r3");

    asm volatile("sc":"=r"(retval):"0"(SC_ISSUPERSTATE):"memory");

    return retval;

    AROS_LIBFUNC_EXIT
}

void *__EXCEPTION_0_Prolog();
void *__EXCEPTION_1_Prolog();
void *__EXCEPTION_2_Prolog();
void *__EXCEPTION_3_Prolog();
void *__EXCEPTION_4_Prolog();
void *__EXCEPTION_5_Prolog();
void *__EXCEPTION_6_Prolog();
void *__EXCEPTION_7_Prolog();
void *__EXCEPTION_8_Prolog();
void *__EXCEPTION_9_Prolog();
void *__EXCEPTION_10_Prolog();
void *__EXCEPTION_11_Prolog();
void *__EXCEPTION_12_Prolog();
void *__EXCEPTION_13_Prolog();
void *__EXCEPTION_14_Prolog();
void *__EXCEPTION_15_Prolog();

void intr_init()
{
    D(bug("[KRN] Setting up exception handlers\n"));
    wrspr(IVPR, ((uint32_t)&__EXCEPTION_0_Prolog) & 0xffff0000);

    wrspr(IVOR0, ((uint32_t)&__EXCEPTION_0_Prolog) & 0x0000fff0);
    wrspr(IVOR1, ((uint32_t)&__EXCEPTION_1_Prolog) & 0x0000fff0);
    wrspr(IVOR2, ((uint32_t)&__EXCEPTION_2_Prolog) & 0x0000fff0);
    wrspr(IVOR3, ((uint32_t)&__EXCEPTION_3_Prolog) & 0x0000fff0);
    wrspr(IVOR4, ((uint32_t)&__EXCEPTION_4_Prolog) & 0x0000fff0);
    wrspr(IVOR5, ((uint32_t)&__EXCEPTION_5_Prolog) & 0x0000fff0);
    wrspr(IVOR6, ((uint32_t)&__EXCEPTION_6_Prolog) & 0x0000fff0);
    wrspr(IVOR7, ((uint32_t)&__EXCEPTION_7_Prolog) & 0x0000fff0);
    wrspr(IVOR8, ((uint32_t)&__EXCEPTION_8_Prolog) & 0x0000fff0);
    wrspr(IVOR9, ((uint32_t)&__EXCEPTION_9_Prolog) & 0x0000fff0);
    wrspr(IVOR10, ((uint32_t)&__EXCEPTION_10_Prolog) & 0x0000fff0);
    wrspr(IVOR11, ((uint32_t)&__EXCEPTION_11_Prolog) & 0x0000fff0);
    wrspr(IVOR12, ((uint32_t)&__EXCEPTION_12_Prolog) & 0x0000fff0);
    wrspr(IVOR13, ((uint32_t)&__EXCEPTION_13_Prolog) & 0x0000fff0);
    wrspr(IVOR14, ((uint32_t)&__EXCEPTION_14_Prolog) & 0x0000fff0);
    wrspr(IVOR15, ((uint32_t)&__EXCEPTION_15_Prolog) & 0x0000fff0);

    /* Disable external interrupts completely */
    wrdcr(UIC0_ER, 0);
    wrdcr(UIC1_ER, 0);
}

#define _STR(x) #x
#define STR(x) _STR(x)
#define PUT_INTR_TEMPLATE(num, handler) \
    asm volatile(".section .text,\"ax\"\n\t.align 5\n\t.globl __EXCEPTION_" STR(num) "_Prolog\n\t.type __EXCEPTION_" STR(num) "_Prolog,@function\n"    \
        "__EXCEPTION_" STR(num) "_Prolog: \n\t"                                                       \
                 "mtsprg1 %%r3          \n\t"   /* save %r3 */                                                  \
                 "mfcr %%r3             \n\t"   /* copy CR to %r3 */                                            \
                 "mtsprg2 %%r3          \n\t"   /* save %r3 */                                                  \
                                                                                                                \
                 "mfsrr1 %%r3           \n\t"   /* srr1 (previous MSR) reg into %r3 */                          \
                 "andi. %%r3,%%r3,%0    \n\t"   /* Was the PR bit set in MSR already? */                        \
                 "beq- 1f               \n\t"   /* No, we were in supervisor mode */                           \
                                                                                                                \
                 "mfsprg0 %%r3          \n\t"   /* user mode case: SSP into %r3 */                              \
                 "b 2f                  \n"                                                                     \
        "1:       mr %%r3,%%r1          \n\t"   /* Supervisor case: use current stack */                        \
        "2:       addi %%r3,%%r3,%1     "                                                                       \
                 ::"i"(MSR_PR),"i"(-sizeof(regs_t))); \
        asm volatile("stw %%r0, %[gpr0](%%r3)  \n\t" \
                  "stw %%r1, %[gpr1](%%r3)  \n\t" \
                  "stw %%r2, %[gpr2](%%r3)  \n\t" \
                  "mfsprg1 %%r0             \n\t" \
                  "stw %%r4, %[gpr4](%%r3)  \n\t" \
                  "stw %%r0, %[gpr3](%%r3)  \n\t" \
                  "stw %%r5, %[gpr5](%%r3)  \n\t" \
                  "mfsprg2 %%r2             \n\t" \
                  "mfsrr0 %%r0              \n\t" \
                  "mfsrr1 %%r1              \n\t" \
                  "lis %%r5, " #handler "@ha\n\t" \
                  "la %%r5, " #handler "@l(%%r5)\n\t" \
                  "li %%r4, %[irq]          \n\t" \
                  "stw %%r2,%[ccr](%%r3)    \n\t" \
                  "stw %%r0,%[srr0](%%r3)   \n\t" \
                  "stw %%r1,%[srr1](%%r3)   \n\t" \
                  "mfctr %%r0               \n\t" \
                  "mflr %%r1                \n\t" \
                  "mfxer %%r2               \n\t" \
                  "stw %%r0,%[ctr](%%r3)    \n\t" \
                  "stw %%r1,%[lr](%%r3)     \n\t" \
                  "stw %%r2,%[xer](%%r3)    \n\t" \
                  \
                  :: \
                  [gpr0]"i"(offsetof(regs_t, gpr[0])), \
                  [gpr1]"i"(offsetof(regs_t, gpr[1])), \
                  [gpr2]"i"(offsetof(regs_t, gpr[2])), \
                  [gpr3]"i"(offsetof(regs_t, gpr[3])), \
                  [gpr4]"i"(offsetof(regs_t, gpr[4])), \
                  [gpr5]"i"(offsetof(regs_t, gpr[5])), \
                  \
                  [ccr]"i"(offsetof(regs_t, ccr)), \
                  [srr0]"i"(offsetof(regs_t, srr0)), \
                  [srr1]"i"(offsetof(regs_t, srr1)), \
                  [ctr]"i"(offsetof(regs_t, ctr)), \
                  [lr]"i"(offsetof(regs_t, lr)), \
                  [xer]"i"(offsetof(regs_t, xer)), \
                  [irq]"i"(num) \
                  ); \
                 /* \
                  * Registers %r0 to %r5 are now saved together with CPU state. Go to the \
                  * trampoline code which will care about the rest. Adjust the stack frame pointer now, \
                  * or else it will be destroyed later by C code. \
                  */ \
                 asm volatile("addi %r1,%r3,-16"); \
                 \
                 /* \
                  * Go to the trampoline code. Use long call within whole 4GB addresspace in order to \
                  * avoid any trouble in future. \
                  */ \
                 asm volatile( "b __EXCEPTION_Trampoline\n\t");

void __attribute__((noreturn)) decrementer_handler(regs_t *ctx, uint8_t exception, void *self)
{
    struct KernelBase *KernelBase = getKernelBase();

    /* Clear the DIS bit - we have received decrementer exception */
    wrspr(TSR, TSR_DIS);
//    D(bug("[KRN] Decrementer handler. Context @ %p. srr1=%08x\n", ctx, ctx->srr1));

    if (!IsListEmpty(&KernelBase->kb_Exceptions[exception]))
    {
        struct IntrNode *in, *in2;

        ForeachNodeSafe(&KernelBase->kb_Exceptions[exception], in, in2)
        {
            if (in->in_Handler)
                in->in_Handler(in->in_HandlerData, in->in_HandlerData2);
        }
    }

    core_ExitInterrupt(ctx);
}


void __attribute__((noreturn)) generic_handler(regs_t *ctx, uint8_t exception, void *self)
{
    struct KernelBase *KernelBase = getKernelBase();
    struct ExecBase *SysBase = getSysBase();

    if (!IsListEmpty(&KernelBase->kb_Exceptions[exception]))
    {
        struct IntrNode *in, *in2;

        ForeachNodeSafe(&KernelBase->kb_Exceptions[exception], in, in2)
        {
            if (in->in_Handler)
                in->in_Handler(in->in_HandlerData, in->in_HandlerData2);
        }
    }

    D(bug("[KRN] Exception %d handler. Context @ %p, SysBase @ %p, KernelBase @ %p\n", exception, ctx, SysBase, KernelBase));
    if (SysBase)
    {
        struct Task *t = FindTask(NULL);
        D(bug("[KRN] %s %p (%s)\n", t->tc_Node.ln_Type == NT_TASK ? "Task":"Process", t, t->tc_Node.ln_Name ? t->tc_Node.ln_Name : "--unknown--"));
    }
    D(bug("[KRN] SRR0=%08x, SRR1=%08x DEAR=%08x ESR=%08x\n",ctx->srr0, ctx->srr1, rdspr(DEAR), rdspr(ESR)));
    D(bug("[KRN] CTR=%08x LR=%08x XER=%08x CCR=%08x\n", ctx->ctr, ctx->lr, ctx->xer, ctx->ccr));
    D(bug("[KRN] DAR=%08x DSISR=%08x\n", ctx->dar, ctx->dsisr));
    D(bug("[KRN] GPR00=%08x GPR01=%08x GPR02=%08x GPR03=%08x\n",
             ctx->gpr[0],ctx->gpr[1],ctx->gpr[2],ctx->gpr[3]));
    D(bug("[KRN] GPR04=%08x GPR05=%08x GPR06=%08x GPR07=%08x\n",
             ctx->gpr[4],ctx->gpr[5],ctx->gpr[6],ctx->gpr[7]));
    D(bug("[KRN] GPR08=%08x GPR09=%08x GPR10=%08x GPR11=%08x\n",
             ctx->gpr[8],ctx->gpr[9],ctx->gpr[10],ctx->gpr[11]));
    D(bug("[KRN] GPR12=%08x GPR13=%08x GPR14=%08x GPR15=%08x\n",
             ctx->gpr[12],ctx->gpr[13],ctx->gpr[14],ctx->gpr[15]));

    D(bug("[KRN] GPR16=%08x GPR17=%08x GPR18=%08x GPR19=%08x\n",
             ctx->gpr[16],ctx->gpr[17],ctx->gpr[18],ctx->gpr[19]));
    D(bug("[KRN] GPR20=%08x GPR21=%08x GPR22=%08x GPR23=%08x\n",
             ctx->gpr[20],ctx->gpr[21],ctx->gpr[22],ctx->gpr[23]));
    D(bug("[KRN] GPR24=%08x GPR25=%08x GPR26=%08x GPR27=%08x\n",
             ctx->gpr[24],ctx->gpr[25],ctx->gpr[26],ctx->gpr[27]));
    D(bug("[KRN] GPR28=%08x GPR29=%08x GPR30=%08x GPR31=%08x\n",
             ctx->gpr[28],ctx->gpr[29],ctx->gpr[30],ctx->gpr[31]));

    D(bug("[KRN] Instruction dump:\n"));
    int i;
    ULONG *p = (ULONG*)ctx->srr0;
    for (i=0; i < 8; i++)
    {
        D(bug("[KRN] %08x: %08x\n", &p[i], p[i]));
    }

    if (IsListEmpty(&KernelBase->kb_Exceptions[exception]))
    {
        D(bug("[KRN] **UNHANDLED EXCEPTION** stopping here...\n"));

        while(1) {
            wrmsr(rdmsr() | MSR_POW);
        }
    }

    core_ExitInterrupt(ctx);
}

void __attribute__((noreturn)) mmu_handler(regs_t *ctx, uint8_t exception, void *self)
{
    struct KernelBase *KernelBase = getKernelBase();

    uint32_t insn = *(uint32_t *)ctx->srr0;

    /* SysBase access at 4UL? Occurs only with lwz instruction and DEAR=4 */
    if ((insn & 0xfc000000) == 0x80000000 && rdspr(DEAR) == 4)
    {
        int reg = (insn & 0x03e00000) >> 21;

//        D(bug("[KRN] Pagefault exception. Someone tries to get SysBase (%08x) from 0x00000004 into r%d. EVIL EVIL EVIL!\n",
//              getSysBase(), reg));

        ctx->gpr[reg] = getSysBase();
        ctx->srr0 += 4;

        core_LeaveInterrupt(ctx);
    }
    else
        generic_handler(ctx, exception, self);
}



static void __attribute__((used)) __EXCEPTION_Prolog_template()
{

    /*
     * Create partial context on the stack. It is impossible to save it fully since the
     * exception handlers have limited size. This code will do as much as possible and then
     * jump to general trampoline...
     */

    PUT_INTR_TEMPLATE(0, generic_handler); /* crit */
    PUT_INTR_TEMPLATE(1, generic_handler);
    PUT_INTR_TEMPLATE(2, generic_handler);
    PUT_INTR_TEMPLATE(3, generic_handler);
    PUT_INTR_TEMPLATE(4, uic_handler);
    PUT_INTR_TEMPLATE(5, generic_handler);
    PUT_INTR_TEMPLATE(6, generic_handler);
    PUT_INTR_TEMPLATE(7, generic_handler);
    PUT_INTR_TEMPLATE(8, syscall_handler);
    PUT_INTR_TEMPLATE(9, generic_handler);
    PUT_INTR_TEMPLATE(10, decrementer_handler);
    PUT_INTR_TEMPLATE(11, generic_handler);
    PUT_INTR_TEMPLATE(12, generic_handler); /* crit */
    PUT_INTR_TEMPLATE(13, mmu_handler);
    PUT_INTR_TEMPLATE(14, generic_handler);
    PUT_INTR_TEMPLATE(15, generic_handler); /* crit */
}

static void __attribute__((used)) __EXCEPTION_Trampoline_template()
{
    asm volatile(".section .text,\"ax\"\n\t.align 5\n\t.globl __EXCEPTION_Trampoline\n\t.type __EXCEPTION_Trampoline,@function\n"
        "__EXCEPTION_Trampoline:            \n\t"
                 "stw %%r6,%[gpr6](%%r3) \n\t"
                 "stw %%r7,%[gpr7](%%r3) \n\t"
                 "stw %%r8,%[gpr8](%%r3) \n\t"
                 "stw %%r9,%[gpr9](%%r3) \n\t"
                 "stw %%r10,%[gpr10](%%r3) \n\t"
                 "stw %%r11,%[gpr11](%%r3) \n\t"
                 "stw %%r12,%[gpr12](%%r3) \n\t"
                 "stw %%r13,%[gpr13](%%r3) \n\t"
                 "stw %%r14,%[gpr14](%%r3) \n\t"
                 "stw %%r15,%[gpr15](%%r3) \n\t"
                 "stw %%r16,%[gpr16](%%r3) \n\t"
                 "stw %%r17,%[gpr17](%%r3) \n\t"
                 "stw %%r18,%[gpr18](%%r3) \n\t"
                 "stw %%r19,%[gpr19](%%r3) \n\t"
                 "stw %%r20,%[gpr20](%%r3) \n\t"
                 "stw %%r21,%[gpr21](%%r3) \n\t"
                 "stw %%r22,%[gpr22](%%r3) \n\t"
                 "stw %%r23,%[gpr23](%%r3) \n\t"
                 "stw %%r24,%[gpr24](%%r3) \n\t"
                 "stw %%r25,%[gpr25](%%r3) \n\t"
                 "stw %%r26,%[gpr26](%%r3) \n\t"
                 "stw %%r27,%[gpr27](%%r3) \n\t"
                 "stw %%r28,%[gpr28](%%r3) \n\t"
                 "stw %%r29,%[gpr29](%%r3) \n\t"
                 "stw %%r30,%[gpr30](%%r3) \n\t"
                 "stw %%r31,%[gpr31](%%r3) \n\t"
                 "mr %%r28,%%r3            \n\t"
                 "mr %%r29,%%r4            \n\t"
                 "mr %%r30,%%r5            \n\t"
                 "mtsrr0 %%r5           \n\t"
                 "lis %%r9, %[msrval]@ha  \n\t"
                 "ori %%r9,%%r9, %[msrval]@l \n\t"
                 "mtsrr1 %%r9              \n\t"
                 "sync; isync; rfi"
                 ::
                 [gpr6]"i"(offsetof(regs_t, gpr[6])),
                 [gpr7]"i"(offsetof(regs_t, gpr[7])),
                 [gpr8]"i"(offsetof(regs_t, gpr[8])),
                 [gpr9]"i"(offsetof(regs_t, gpr[9])),
                 [gpr10]"i"(offsetof(regs_t, gpr[10])),
                 [gpr11]"i"(offsetof(regs_t, gpr[11])),
                 [gpr12]"i"(offsetof(regs_t, gpr[12])),
                 [gpr13]"i"(offsetof(regs_t, gpr[13])),
                 [gpr14]"i"(offsetof(regs_t, gpr[14])),
                 [gpr15]"i"(offsetof(regs_t, gpr[15])),
                 [gpr16]"i"(offsetof(regs_t, gpr[16])),
                 [gpr17]"i"(offsetof(regs_t, gpr[17])),
                 [gpr18]"i"(offsetof(regs_t, gpr[18])),
                 [gpr19]"i"(offsetof(regs_t, gpr[19])),
                 [gpr20]"i"(offsetof(regs_t, gpr[20])),
                 [gpr21]"i"(offsetof(regs_t, gpr[21])),
                 [gpr22]"i"(offsetof(regs_t, gpr[22])),
                 [gpr23]"i"(offsetof(regs_t, gpr[23])),
                 [gpr24]"i"(offsetof(regs_t, gpr[24])),
                 [gpr25]"i"(offsetof(regs_t, gpr[25])),
                 [gpr26]"i"(offsetof(regs_t, gpr[26])),
                 [gpr27]"i"(offsetof(regs_t, gpr[27])),
                 [gpr28]"i"(offsetof(regs_t, gpr[28])),
                 [gpr29]"i"(offsetof(regs_t, gpr[29])),
                 [gpr30]"i"(offsetof(regs_t, gpr[30])),
                 [gpr31]"i"(offsetof(regs_t, gpr[31])),
                 [msrval]"i"(MSR_ME|MSR_CE|MSR_FP)
    );
}


static void __attribute__((used)) __core_LeaveInterrupt()
{
    asm volatile(".section .text,\"ax\"\n\t.align 5\n\t.globl core_LeaveInterrupt\n\t.type core_LeaveInterrupt,@function\n"
        "core_LeaveInterrupt:            \n\t"
                 "lwz %%r31,%[gpr31](%%r3)      \n\t"
                 "lwz %%r30,%[gpr30](%%r3)      \n\t"
                 "lwz %%r29,%[gpr29](%%r3)      \n\t"
                 "lwz %%r28,%[gpr28](%%r3)      \n\t"
                 "lwz %%r27,%[gpr27](%%r3)      \n\t"
                 "lwz %%r26,%[gpr26](%%r3)      \n\t"
                 "lwz %%r25,%[gpr25](%%r3)      \n\t"
                 "lwz %%r24,%[gpr24](%%r3)      \n\t"
                 "lwz %%r23,%[gpr23](%%r3)      \n\t"
                 "lwz %%r22,%[gpr22](%%r3)      \n\t"
                 "lwz %%r21,%[gpr21](%%r3)      \n\t"
                 "lwz %%r20,%[gpr20](%%r3)      \n\t"
                 "lwz %%r19,%[gpr19](%%r3)      \n\t"
                 "lwz %%r18,%[gpr18](%%r3)      \n\t"
                 "lwz %%r17,%[gpr17](%%r3)      \n\t"
                 "lwz %%r16,%[gpr16](%%r3)      \n\t"
                 "lwz %%r15,%[gpr15](%%r3)      \n\t"
                 "lwz %%r14,%[gpr14](%%r3)      \n\t"
                 "lwz %%r13,%[gpr13](%%r3)      \n\t"
                 "lwz %%r12,%[gpr12](%%r3)      \n\t"
        ::
        [gpr12]"i"(offsetof(regs_t, gpr[12])),
        [gpr13]"i"(offsetof(regs_t, gpr[13])),
        [gpr14]"i"(offsetof(regs_t, gpr[14])),
        [gpr15]"i"(offsetof(regs_t, gpr[15])),
        [gpr16]"i"(offsetof(regs_t, gpr[16])),
        [gpr17]"i"(offsetof(regs_t, gpr[17])),
        [gpr18]"i"(offsetof(regs_t, gpr[18])),
        [gpr19]"i"(offsetof(regs_t, gpr[19])),
        [gpr20]"i"(offsetof(regs_t, gpr[20])),
        [gpr21]"i"(offsetof(regs_t, gpr[21])),
        [gpr22]"i"(offsetof(regs_t, gpr[22])),
        [gpr23]"i"(offsetof(regs_t, gpr[23])),
        [gpr24]"i"(offsetof(regs_t, gpr[24])),
        [gpr25]"i"(offsetof(regs_t, gpr[25])),
        [gpr26]"i"(offsetof(regs_t, gpr[26])),
        [gpr27]"i"(offsetof(regs_t, gpr[27])),
        [gpr28]"i"(offsetof(regs_t, gpr[28])),
        [gpr29]"i"(offsetof(regs_t, gpr[29])),
        [gpr30]"i"(offsetof(regs_t, gpr[30])),
        [gpr31]"i"(offsetof(regs_t, gpr[31]))
        );

    asm volatile(
                 "lwz %%r11,%[gpr11](%%r3)      \n\t"
                 "lwz %%r0,%[srr0](%%r3)        \n\t"
                 "mtsrr0 %%r0                   \n\t"
                 "lwz %%r0,%[srr1](%%r3)        \n\t"
                 //"rlwinm %%r0,%%r0,0,14,12      \n\t"
                 "mtsrr1 %%r0                   \n\t"
                 "lwz %%r0,%[ctr](%%r3)         \n\t"
                 "mtctr %%r0                    \n\t"
                 "lwz %%r0,%[lr](%%r3)          \n\t"
                 "mtlr %%r0                     \n\t"
                 "lwz %%r0,%[xer](%%r3)         \n\t"
                 "mtxer %%r0                    \n\t"
                 "lwz %%r10,%[gpr10](%%r3)      \n\t"
                 "lwz %%r9,%[gpr9](%%r3)        \n\t"
                 "lwz %%r8,%[gpr8](%%r3)        \n\t"
                 "lwz %%r7,%[gpr7](%%r3)        \n\t"
                 "lwz %%r6,%[gpr6](%%r3)        \n\t"
                 "lwz %%r5,%[gpr5](%%r3)        \n\t"
                 "lwz %%r4,%[gpr4](%%r3)        \n\t"
                 "lwz %%r0,%[gpr3](%%r3)        \n\t"
                 "mtsprg1 %%r0                  \n\t"
                 "lwz %%r2,%[gpr2](%%r3)        \n\t"
                 "stwcx. %%r0,0,%%r1            \n\t"
                 "lwz %%r0,%[ccr](%%r3)         \n\t"
                 "mtcr %%r0                     \n\t"
                 "lwz %%r1,%[gpr1](%%r3)        \n\t"
                 "lwz %%r0,%[gpr0](%%r3)        \n\t"
                 "mfsprg1 %%r3                  \n\t"
                 "sync; isync; rfi"
        ::
        [ccr]"i"(offsetof(regs_t, ccr)),        /* */
        [srr0]"i"(offsetof(regs_t, srr0)),      /* */
        [srr1]"i"(offsetof(regs_t, srr1)),/* */
        [ctr]"i"(offsetof(regs_t, ctr)),/**/
        [lr]"i"(offsetof(regs_t, lr)),/**/
        [xer]"i"(offsetof(regs_t, xer)),
        [gpr0]"i"(offsetof(regs_t, gpr[0])),
        [gpr1]"i"(offsetof(regs_t, gpr[1])),
        [gpr2]"i"(offsetof(regs_t, gpr[2])),
        [gpr3]"i"(offsetof(regs_t, gpr[3])),
        [gpr4]"i"(offsetof(regs_t, gpr[4])),
        [gpr5]"i"(offsetof(regs_t, gpr[5])),
        [gpr6]"i"(offsetof(regs_t, gpr[6])),
        [gpr7]"i"(offsetof(regs_t, gpr[7])),
        [gpr8]"i"(offsetof(regs_t, gpr[8])),
        [gpr9]"i"(offsetof(regs_t, gpr[9])),
        [gpr10]"i"(offsetof(regs_t, gpr[10])),
        [gpr11]"i"(offsetof(regs_t, gpr[11]))
    );
}
