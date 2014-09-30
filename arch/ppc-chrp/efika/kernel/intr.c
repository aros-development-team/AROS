/*
    Copyright © 2008-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <inttypes.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <asm/mpc5200b.h>
#include <stddef.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel_intern.h"
#include "syscall.h"

extern void __tmpl_start();
extern uint32_t __tmpl_addr_lo;
extern uint32_t __tmpl_addr_hi;
extern uint32_t __tmpl_irq_num;
extern uint32_t __tmpl_length;
static void init_interrupt(uint8_t num, void *handler);
void __attribute__((noreturn)) program_handler(regs_t *ctx, uint8_t exception, void *self);
void __attribute__((noreturn)) generic_handler(regs_t *ctx, uint8_t exception, void *self);
void __attribute__((noreturn)) decrementer_handler(regs_t *ctx, uint8_t exception, void *self);
static void flush_cache(char *start, char *end);

AROS_LH4(void *, KrnAddExceptionHandler,
         AROS_LHA(uint8_t, irq, D0),
         AROS_LHA(void *, handler, A0),
         AROS_LHA(void *, handlerData, A1),
         AROS_LHA(void *, handlerData2, A2),
         struct KernelBase *, KernelBase, 14, Kernel)
{
    AROS_LIBFUNC_INIT

    struct ExecBase *SysBase = getSysBase();
    struct ExceptNode *handle = NULL;
    D(bug("[KRN] KrnAddExceptionHandler(%02x, %012p, %012p, %012p):\n", irq, handler, handlerData, handlerData2));

    if (irq < 21)
    {
        /* Go to supervisor mode */
        goSuper();

        handle = Allocate(KernelBase->kb_SupervisorMem, sizeof(struct ExceptNode));
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
         struct KernelBase *, KernelBase, 15, Kernel)
{
    AROS_LIBFUNC_INIT

    struct ExecBase *SysBase = getSysBase();
    struct ExceptNode *h = handle;

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

            ictl_enable_irq(irq);

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
        	ictl_disable_irq(irq);
        }
        Enable();

        Deallocate(KernelBase->kb_SupervisorMem, h, sizeof(struct IntrNode));

        goUser();
    }

    AROS_LIBFUNC_EXIT
}


/*
 * G2 core and exceptions.
 *
 * The MPC5200B CPU just like most of the PowerPC family members, has two fixed
 * locations for exception handlers: 0x0000_xxxx and 0xFFFF_xxxx. When an
 * exception occurs, CPU calculates the entry address by shifting exception
 * number by eight bits left, adds the base location and begins execution of the
 * code at calculated address.
 *
 * For AROS it means that it has only 256 bytes (64 instructions) for an
 * exception entry code. Therefore, just like all PowerPC operating systems,
 * AROS performs a quick initialization of an exception, saves few registers,
 * sets the exception number and determines the handler address and then it jumps
 * to a common trampoline code. There, the rest of the CPU context is saved, MMU
 * is activated and the handler routine written in C is called.
 *
 * Leaving the exception is performed through core_LeaveInterrupt() call which
 * takes the CPU context as parameter.
 *
 * About MMU.
 *
 * There is some trouble related to MMU, location of exception handler and
 * accessing data from there. PPC executes exception handlers in real mode, which
 * means that the MMU translations are disabled completely. Therefore, going back
 * to MMU-ed state is performed in two steps:
 * 1. The MMU for DATA is turned on very early, because otherwise the exception
 *    handler wouldn't be able to access the supervisor stack
 * 2. The MMU for CODE is turned on as late as possible. It happens when the C
 *    code is called. The Call is performed through the rfi instruction, which
 *    restores the old contents of MSR register (actually it is prepared by the
 *    asm part of exception handler) which in turn enables MMU for CODE.
 */

extern uint32_t __vector_imiss;
extern uint32_t __vector_dmiss;
extern uint32_t __vector_dmissw;

void intr_init()
{
	D(bug("[KRN] Initializing exception handlers\n"));

	init_interrupt( 1, generic_handler);	/* RESET */
	init_interrupt( 2, generic_handler);	/* Machine check */
	init_interrupt( 3, mmu_handler);		/* DSI */
	init_interrupt( 4, mmu_handler);		/* ISI */
	init_interrupt( 5, ictl_handler);		/* External Intr */
	init_interrupt( 6, generic_handler);	/* Alignment */
	init_interrupt( 7, program_handler);	/* Program */
	init_interrupt( 8, generic_handler);	/* Floating point unavailable */
	init_interrupt( 9, decrementer_handler);/* Decrementer */
	init_interrupt(10, generic_handler);	/* critical exception */
	init_interrupt(12, syscall_handler);	/* Syscall */
	init_interrupt(13, generic_handler);	/* Trace */
	init_interrupt(16, generic_handler);	/* Instruction translation miss */
	init_interrupt(17, generic_handler);	/* Data load translation miss */
	init_interrupt(18, generic_handler);	/* Data store translation miss */
	init_interrupt(19, generic_handler);	/* Instruction address breakpoint */
	init_interrupt(20, ictl_handler);		/* SMI */
}

/*
 * Initializer of an exception handler. It copies the template code into proper
 * location and adjust two exception-dependent elements in the code - the
 * instruction which loads exception number: "li %r4,exception_number" and the
 * two instructions which load the address of a handler:
 * "lis %r5,handler_address@ha; la %r5, handler_address@l(%r5)"
 *
 * Once ready, data cache has to be flushed back into memory and the instruction
 * cache has to be invalidated.
 */
static void init_interrupt(uint8_t num, void *handler)
{
	if (num > 0 && num < 0x2f)
	{
		intptr_t target = num << 8;

		if (num == 16)
			memcpy((void*)target, &__vector_imiss, 256);
		else if (num == 17)
			memcpy((void*)target, &__vector_dmiss, 256);
		else if (num == 18)
			memcpy((void*)target, &__vector_dmissw, 256);
		else
		{
			memcpy((void*)target, __tmpl_start, __tmpl_length);

			/* Fix the exception  number */
			*(uint16_t *)(target + __tmpl_irq_num) = num;

			/* Fix the handler address */
			*(uint16_t *)(target + __tmpl_addr_lo) = (intptr_t)handler & 0x0000ffff;
			*(uint16_t *)(target + __tmpl_addr_hi) = (intptr_t)handler >> 16;

			/*
			 * Adjustment of the lower halfword of address is done through "la"
			 * instruction, which happens to be the same as addi:
			 *
			 * "la %reg1, offset(%reg2) <=> addi %reg1, %reg1, offset"
			 *
			 * If the offset is bigger then 32KB (thus seen by addi as a negative
			 * number), increase the upper halfword by one.
			 */
			if ((intptr_t)handler & 0x00008000)
				(*(uint16_t *)(target + __tmpl_addr_hi))++;
		}

		/* Flush the cache */
		flush_cache((char*)target, (char*)target + 0xff);
	}
}

/* Tiny routine to flush caches for a region of memory */
static void flush_cache(char *start, char *end)
{
    start = (char*)((unsigned long)start & 0xffffffe0);
    end = (char*)((unsigned long)end & 0xffffffe0);
    char *ptr;

    for (ptr = start; ptr < end; ptr +=32)
    {
        asm volatile("dcbst 0,%0"::"r"(ptr));
    }
    asm volatile("sync");

    for (ptr = start; ptr < end; ptr +=32)
    {
        asm volatile("icbi 0,%0"::"r"(ptr));
    }

    asm volatile("sync; isync; ");
}

/* FPU handler */
void __attribute__((noreturn)) fpu_handler(context_t *ctx, uint8_t exception, void *self)
{
    struct KernelBase *KernelBase = getKernelBase();

    if (KernelBase)
    {
    	if (!IsListEmpty(&KernelBase->kb_Exceptions[exception]))
    	{
			struct ExceptNode *in, *intemp;

			ForeachNodeSafe(&KernelBase->kb_Exceptions[exception], in, intemp)
			{
				/*
				 * call every handler tied to this exception.
				 */
				if (in->in_Handler)
					in->in_Handler(ctx, in->in_HandlerData, in->in_HandlerData2);
			}
    	}
    }

    core_LeaveInterrupt(ctx);
}

extern uint64_t tbu1;
extern uint64_t tbu2;
extern uint64_t last_calc;
extern uint64_t idle_time;
extern uint32_t cpu_usage;
extern struct Task *idle_task;

/* Decrementer handler */
void __attribute__((noreturn)) decrementer_handler(regs_t *ctx, uint8_t exception, void *self)
{
    struct KernelBase *KernelBase = getKernelBase();
    struct ExecBase *SysBase = getSysBase();
//    static uint32_t cnt = 0;

    asm volatile("mtdec %0"::"r"(33000000/100));

    if (KernelBase)
    {
    	if (!IsListEmpty(&KernelBase->kb_Exceptions[exception]))
    	{
			struct ExceptNode *in, *intemp;

			ForeachNodeSafe(&KernelBase->kb_Exceptions[exception], in, intemp)
			{
				/*
				 * call every handler tied to this exception.
				 */
				if (in->in_Handler)
					in->in_Handler(ctx, in->in_HandlerData, in->in_HandlerData2);
			}
    	}
    }

    if (SysBase && SysBase->Elapsed)
    {
        if (--SysBase->Elapsed == 0)
        {
        	SysBase->SysFlags |= 0x2000;
        	SysBase->AttnResched |= 0x80;
        }
    }

    /* CPU usage meter. it should not be here, actually */
    uint64_t current = mftbu();
	if (current - last_calc > 33000000)
	{
		uint32_t total_time = current - last_calc;

		if (SysBase->ThisTask == idle_task)
		{
			tbu2 = mftbu();
			idle_time += tbu2 - tbu1;
			tbu1 = tbu2;
		}

		if (total_time < idle_time)
			total_time=idle_time;

		cpu_usage = 1000 - ((uint32_t)(idle_time))/(total_time /1000);

		D(bug("[KRN] CPU usage: %3d.%d\n", cpu_usage / 10, cpu_usage % 10));

		last_calc = current;
		idle_time = 0;
	}

    core_ExitInterrupt(ctx);
}

/* Generic boring handler */
void __attribute__((noreturn)) program_handler(regs_t *ctx, uint8_t exception, void *self)
{
    struct KernelBase *KernelBase = getKernelBase();
    struct ExecBase *SysBase = getSysBase();
    int handled = 0;

    uint32_t insn = *(uint32_t *)ctx->srr0;

    if ((insn & 0xfc1fffff) == 0x7c1442a6) /* mfspr sprg4 */
    {
    	ctx->gpr[(insn >> 21) & 0x1f] = getKernelBase();
    	ctx->srr0 += 4;
    	core_LeaveInterrupt(ctx);
    }
    else if ((insn & 0xfc1fffff) == 0x7c1542a6) /* mfspr sprg5 */
    {
    	ctx->gpr[(insn >> 21) & 0x1f] = getSysBase();
    	ctx->srr0 += 4;
    	core_LeaveInterrupt(ctx);
    }
    else if (insn == 0x7fe00008)
    {
    	D(bug("[KRN] trap @ %08x (r3=%08x)\n", ctx->srr0, ctx->gpr[3]));

    	if (SysBase)
        {
            struct Task *t = FindTask(NULL);
            D(bug("[KRN] %s %p (%s)\n", t->tc_Node.ln_Type == NT_TASK ? "Task":"Process", t, t->tc_Node.ln_Name ? t->tc_Node.ln_Name : "--unknown--"));
        }
        D(bug("[KRN] SRR0=%08x, SRR1=%08x\n",ctx->srr0, ctx->srr1));
        D(bug("[KRN] CTR=%08x LR=%08x XER=%08x CCR=%08x\n", ctx->ctr, ctx->lr, ctx->xer, ctx->ccr));
        D(bug("[KRN] DAR=%08x DSISR=%08x\n", ctx->dar, ctx->dsisr));

        D(bug("[KRN] HASH1=%08x HASH2=%08x IMISS=%08x DMISS=%08x ICMP=%08x DCMP=%08x\n",
            		rdspr(978), rdspr(979), rdspr(980), rdspr(976), rdspr(981), rdspr(977)));

        D(bug("[KRN] SPRG0=%08x SPRG1=%08x SPRG2=%08x SPRG3=%08x SPRG4=%08x SPRG5=%08x\n",
        		rdspr(SPRG0),rdspr(SPRG1),rdspr(SPRG2),rdspr(SPRG3),rdspr(SPRG4),rdspr(SPRG5)));

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

    	ctx->srr0 += 4;
    	core_LeaveInterrupt(ctx);
    }
    else
    	generic_handler(ctx, exception, self);
}



/* Generic boring handler */
void __attribute__((noreturn)) generic_handler(regs_t *ctx, uint8_t exception, void *self)
{
    struct KernelBase *KernelBase = getKernelBase();
    struct ExecBase *SysBase = getSysBase();
    int handled = 0;

    if (KernelBase)
    {
    	if (!IsListEmpty(&KernelBase->kb_Exceptions[exception]))
    	{
			struct ExceptNode *in, *intemp;

			ForeachNodeSafe(&KernelBase->kb_Exceptions[exception], in, intemp)
			{
				/*
				 * call every handler tied to this exception. If any of them
				 * returns a non-zero value, the exception is considered handled.
				 *
				 * If no handler will return zero, or there are no handlers at all,
				 * this generic handler will stop cpu.
				 */
				if (in->in_Handler)
					handled |= in->in_Handler(ctx, in->in_HandlerData, in->in_HandlerData2);
			}
    	}
    }

    D(bug("[KRN] Exception %d handler. Context @ %p, SysBase @ %p, KernelBase @ %p\n", exception, ctx, SysBase, KernelBase));
    if (SysBase)
    {
        struct Task *t = FindTask(NULL);
        D(bug("[KRN] %s %p (%s)\n", t->tc_Node.ln_Type == NT_TASK ? "Task":"Process", t, t->tc_Node.ln_Name ? t->tc_Node.ln_Name : "--unknown--"));
    }
    D(bug("[KRN] SRR0=%08x, SRR1=%08x\n",ctx->srr0, ctx->srr1));
    D(bug("[KRN] CTR=%08x LR=%08x XER=%08x CCR=%08x\n", ctx->ctr, ctx->lr, ctx->xer, ctx->ccr));
    D(bug("[KRN] DAR=%08x DSISR=%08x\n", ctx->dar, ctx->dsisr));

    D(bug("[KRN] HASH1=%08x HASH2=%08x IMISS=%08x DMISS=%08x ICMP=%08x DCMP=%08x\n",
        		rdspr(978), rdspr(979), rdspr(980), rdspr(976), rdspr(981), rdspr(977)));

    D(bug("[KRN] SPRG0=%08x SPRG1=%08x SPRG2=%08x SPRG3=%08x SPRG4=%08x SPRG5=%08x\n",
    		rdspr(SPRG0),rdspr(SPRG1),rdspr(SPRG2),rdspr(SPRG3),rdspr(SPRG4),rdspr(SPRG5)));

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

    if (!handled)
    {
		D(bug("[KRN] **UNHANDLED EXCEPTION** stopping here...\n"));

		while(1) {
			wrmsr(rdmsr() | MSR_POW);
		}
    }
    core_LeaveInterrupt(ctx);
}

/*
 * Template code for an exception handler. Packed into a static void function
 * in order to make the assembler constrains usable.
 */
static void __attribute__((used)) __exception_template()
{
	asm volatile(".globl __tmpl_start; .type __tmpl_start,@function\n"
"__tmpl_start:					\n"
"		mtsprg1 %%r3          	\n"   /* save %r3 */
"       mfcr %%r3               \n"   /* copy CR to %r3 */
"       mtsprg3 %%r3            \n"   /* save %r3 */

"       mfmsr %%r3              \n"
"		ori %%r3,%%r3,%2        \n"	  /* Enable address translation for data */
"		mtmsr %%r3				\n"
"       sync; isync             \n"

"       mfsrr1 %%r3             \n"   /* srr1 (previous MSR) reg into %r3 */
"       andi. %%r3,%%r3,%0      \n"   /* Was the PR bit set in MSR already? */
"       beq- 1f                 \n"   /* No, we were in supervisor mode */

"       mfsprg0 %%r3            \n"   /* user mode case: SSP into %r3 */
"       b 2f                    \n"
"1:     mr %%r3,%%r1            \n"   /* Supervisor case: use current stack */
"2:     addi %%r3,%%r3,%1       \n"


::"i"(MSR_PR),"i"(-sizeof(context_t)),"i"(MSR_DS));

    asm volatile(
"		stw %%r0, %[gpr0](%%r3) \n"   /* Store bunch of registers already. I could */
"		stw %%r1, %[gpr1](%%r3) \n"   /* do it in common trampoline code, but it */
"		stw %%r2, %[gpr2](%%r3) \n"   /* is much more sexy to do it here - this code */
"		mfsprg1 %%r0            \n"   /* occupies in theory ZERO bytes in memory */
"		stw %%r4, %[gpr4](%%r3) \n"   /* because the exception vector is 256 bytes long */
"		stw %%r0, %[gpr3](%%r3) \n"   /* and shouldn't be used to anything else than */
"		stw %%r5, %[gpr5](%%r3) \n"   /* exception handler anyway ;) */
"		mfsprg3 %%r2            \n"
"		mfsrr0 %%r0             \n"
"		mfsrr1 %%r1             \n"
"__addr_hi:	lis %%r5, 0xdeadbeef@ha\n"		/* Load the address of an generic handler */
"__addr_lo:	la %%r5, 0xdeadbeef@l(%%r5)\n"	/* yes, load immediate sucks. Think about 64-bit PPC ;) */
"__irq_num:	li %%r4, 0x5a5a     \n"	  /* Load the exception number */
"		stw %%r2,%[ccr](%%r3)   \n"
"		stw %%r0,%[srr0](%%r3)  \n"
"		stw %%r1,%[srr1](%%r3)  \n"
"		mfctr %%r0              \n"
"		mflr %%r1               \n"
"		mfxer %%r2              \n"
"		stw %%r0,%[ctr](%%r3)   \n"
"		stw %%r1,%[lr](%%r3)    \n"
"		stw %%r2,%[xer](%%r3)   \n"


::[gpr0]"i"(offsetof(regs_t, gpr[0])),
  [gpr1]"i"(offsetof(regs_t, gpr[1])),
  [gpr2]"i"(offsetof(regs_t, gpr[2])),
  [gpr3]"i"(offsetof(regs_t, gpr[3])),
  [gpr4]"i"(offsetof(regs_t, gpr[4])),
  [gpr5]"i"(offsetof(regs_t, gpr[5])),
  [ccr]"i"(offsetof(regs_t, ccr)),
  [srr0]"i"(offsetof(regs_t, srr0)),
  [srr1]"i"(offsetof(regs_t, srr1)),
  [ctr]"i"(offsetof(regs_t, ctr)),
  [lr]"i"(offsetof(regs_t, lr)),
  [xer]"i"(offsetof(regs_t, xer)));

    /*
     * Registers %r0 to %r5 are now saved together with CPU state. Go to the
     * trampoline code which will care about the rest. Adjust the stack frame pointer now,
     * or else it will be destroyed later by C code.
     */
    asm volatile("addi %r1,%r3,-16");

    /*
     * Go to the trampoline code. Use long call within whole 4GB addresspace in order to
     * avoid any trouble in future. Moreover use the PHYSICAL address since at this stage
     * MMU for code is still not running! If one would like to use MMU at this stage
     * already, we would have to make region 0x00000000-0x00003000 *EXECUTABLE* :))
     */
    asm volatile( "lis %r2,(__EXCEPTION_Trampoline - " STR(KERNEL_VIRT_BASE) " + " STR(KERNEL_PHYS_BASE) ")@ha;"
				  "la %r2,(__EXCEPTION_Trampoline - " STR(KERNEL_VIRT_BASE) " + " STR(KERNEL_PHYS_BASE) ")@l(%r2); mtctr %r2;");

    /* Jump to the trampoline code */
    asm volatile("bctr;");

    /*
     * Few variables: length of the code above and offsets used to fix the
     * exception number and handler address.
     */
    asm volatile("__tmpl_length: .long . - __tmpl_start\n");
    asm volatile("__tmpl_addr_lo: .long 2 + __addr_lo - __tmpl_start\n");
    asm volatile("__tmpl_addr_hi: .long 2 + __addr_hi - __tmpl_start\n");
    asm volatile("__tmpl_irq_num: .long 2 + __irq_num - __tmpl_start\n");
}

/*
 * Trampoline code is boring. It stores rest of the CPU context and prepares
 * everything for execution of the C code.
 *
 * The only interesting part is the jump to C routine which is done throuhg the
 * rfi instruction (return from interrupt). I do it so because this way I may
 * enable the MMU for code and jump CPU to the desired address within one insn.
 */
static void __attribute__((used)) __EXCEPTION_Trampoline_template()
{
    asm volatile(".section .aros.init,\"ax\"\n\t.align 5\n\t.globl __EXCEPTION_Trampoline\n\t.type __EXCEPTION_Trampoline,@function\n"
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
                 [gpr31]"i"(offsetof(regs_t, gpr[31]))
    );
    asm volatile(
				"mfmsr %%r0						\n\t"
				"ori %%r0,%%r0, %[msrval]@l \n\t"
				"mtmsr %%r0; isync				\n\t"
				"stfd %%f0,%[fr0](%%r3)		\n\t"
				"mffs %%f0						\n\t"
				"stfd %%f0,%[fpscr](%%r3)		\n\t"
				"stfd %%f1,%[fr1](%%r3)		\n\t"
				"stfd %%f2,%[fr2](%%r3)		\n\t"
				"stfd %%f3,%[fr3](%%r3)		\n\t"
				"stfd %%f4,%[fr4](%%r3)		\n\t"
				"stfd %%f5,%[fr5](%%r3)		\n\t"
				"stfd %%f6,%[fr6](%%r3)		\n\t"
				"stfd %%f7,%[fr7](%%r3)		\n\t"
				"stfd %%f8,%[fr8](%%r3)		\n\t"
				"stfd %%f9,%[fr9](%%r3)		\n\t"
				"stfd %%f10,%[fr10](%%r3)		\n\t"
				"stfd %%f11,%[fr11](%%r3)		\n\t"
				"stfd %%f12,%[fr12](%%r3)		\n\t"
				"stfd %%f13,%[fr13](%%r3)		\n\t"
				"stfd %%f14,%[fr14](%%r3)		\n\t"
				"stfd %%f15,%[fr15](%%r3)		\n\t"
				::
				[fpscr]"i"(offsetof(context_t, fpu.fpscr)),
				[fr0]"i"(offsetof(context_t, fpu.fpr[0])),
				[fr1]"i"(offsetof(context_t, fpu.fpr[1])),
				[fr2]"i"(offsetof(context_t, fpu.fpr[2])),
				[fr3]"i"(offsetof(context_t, fpu.fpr[3])),
				[fr4]"i"(offsetof(context_t, fpu.fpr[4])),
				[fr5]"i"(offsetof(context_t, fpu.fpr[5])),
				[fr6]"i"(offsetof(context_t, fpu.fpr[6])),
				[fr7]"i"(offsetof(context_t, fpu.fpr[7])),
				[fr8]"i"(offsetof(context_t, fpu.fpr[8])),
				[fr9]"i"(offsetof(context_t, fpu.fpr[9])),
				[fr10]"i"(offsetof(context_t, fpu.fpr[10])),
				[fr11]"i"(offsetof(context_t, fpu.fpr[11])),
				[fr12]"i"(offsetof(context_t, fpu.fpr[12])),
				[fr13]"i"(offsetof(context_t, fpu.fpr[13])),
				[fr14]"i"(offsetof(context_t, fpu.fpr[14])),
				[fr15]"i"(offsetof(context_t, fpu.fpr[15])),
				[msrval]"i"(MSR_FP)
    );
	asm volatile(
			"stfd %%f16,%[fr16](%%r3)		\n\t"
			"stfd %%f17,%[fr17](%%r3)		\n\t"
			"stfd %%f18,%[fr18](%%r3)		\n\t"
			"stfd %%f19,%[fr19](%%r3)		\n\t"
			"stfd %%f20,%[fr20](%%r3)		\n\t"
			"stfd %%f21,%[fr21](%%r3)		\n\t"
			"stfd %%f22,%[fr22](%%r3)		\n\t"
			"stfd %%f23,%[fr23](%%r3)		\n\t"
			"stfd %%f24,%[fr24](%%r3)		\n\t"
			"stfd %%f25,%[fr25](%%r3)		\n\t"
			"stfd %%f26,%[fr26](%%r3)		\n\t"
			"stfd %%f27,%[fr27](%%r3)		\n\t"
			"stfd %%f28,%[fr28](%%r3)		\n\t"
			"stfd %%f29,%[fr29](%%r3)		\n\t"
			"stfd %%f30,%[fr30](%%r3)		\n\t"
			"stfd %%f31,%[fr31](%%r3)		\n\t"
				"mr %%r28,%%r3            \n\t"
				"mr %%r29,%%r4            \n\t"
				"mr %%r30,%%r5            \n\t"
				"mtsrr0 %%r5           \n\t"
				"lis %%r9, %[msrval]@ha  \n\t"
				"ori %%r9,%%r9, %[msrval]@l \n\t"
				"mtsrr1 %%r9              \n\t"
				"sync; isync; rfi"
				::
				[fr16]"i"(offsetof(context_t, fpu.fpr[16])),
				[fr17]"i"(offsetof(context_t, fpu.fpr[17])),
				[fr18]"i"(offsetof(context_t, fpu.fpr[18])),
				[fr19]"i"(offsetof(context_t, fpu.fpr[19])),
				[fr20]"i"(offsetof(context_t, fpu.fpr[20])),
				[fr21]"i"(offsetof(context_t, fpu.fpr[21])),
				[fr22]"i"(offsetof(context_t, fpu.fpr[22])),
				[fr23]"i"(offsetof(context_t, fpu.fpr[23])),
				[fr24]"i"(offsetof(context_t, fpu.fpr[24])),
				[fr25]"i"(offsetof(context_t, fpu.fpr[25])),
				[fr26]"i"(offsetof(context_t, fpu.fpr[26])),
				[fr27]"i"(offsetof(context_t, fpu.fpr[27])),
				[fr28]"i"(offsetof(context_t, fpu.fpr[28])),
				[fr29]"i"(offsetof(context_t, fpu.fpr[29])),
				[fr30]"i"(offsetof(context_t, fpu.fpr[30])),
				[fr31]"i"(offsetof(context_t, fpu.fpr[31])),
                [msrval]"i"(MSR_ME|MSR_FP|MSR_IS|MSR_DS)

	);
}

/*
 * Return from interrupt - restores the context passed as a parameter in %r3
 * register.
 */
static void __attribute__((used)) __core_LeaveInterrupt()
{
    asm volatile(".section .aros.init,\"ax\"\n\t.align 5\n\t.globl core_LeaveInterrupt\n\t.type core_LeaveInterrupt,@function\n"
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
				"lfd  %%f0,%[fpscr](%%r3)		\n\t"
				"mtfsf 255,%%f0						\n\t"
				"lfd %%f0,%[fr0](%%r3)		\n\t"
				"lfd %%f1,%[fr1](%%r3)		\n\t"
				"lfd %%f2,%[fr2](%%r3)		\n\t"
				"lfd %%f3,%[fr3](%%r3)		\n\t"
				"lfd %%f4,%[fr4](%%r3)		\n\t"
				"lfd %%f5,%[fr5](%%r3)		\n\t"
				"lfd %%f6,%[fr6](%%r3)		\n\t"
				"lfd %%f7,%[fr7](%%r3)		\n\t"
				"lfd %%f8,%[fr8](%%r3)		\n\t"
				"lfd %%f9,%[fr9](%%r3)		\n\t"
				"lfd %%f10,%[fr10](%%r3)		\n\t"
				"lfd %%f11,%[fr11](%%r3)		\n\t"
				"lfd %%f12,%[fr12](%%r3)		\n\t"
				"lfd %%f13,%[fr13](%%r3)		\n\t"
				"lfd %%f14,%[fr14](%%r3)		\n\t"
				"lfd %%f15,%[fr15](%%r3)		\n\t"
				::
				[fpscr]"i"(offsetof(context_t, fpu.fpscr)),
				[fr0]"i"(offsetof(context_t, fpu.fpr[0])),
				[fr1]"i"(offsetof(context_t, fpu.fpr[1])),
				[fr2]"i"(offsetof(context_t, fpu.fpr[2])),
				[fr3]"i"(offsetof(context_t, fpu.fpr[3])),
				[fr4]"i"(offsetof(context_t, fpu.fpr[4])),
				[fr5]"i"(offsetof(context_t, fpu.fpr[5])),
				[fr6]"i"(offsetof(context_t, fpu.fpr[6])),
				[fr7]"i"(offsetof(context_t, fpu.fpr[7])),
				[fr8]"i"(offsetof(context_t, fpu.fpr[8])),
				[fr9]"i"(offsetof(context_t, fpu.fpr[9])),
				[fr10]"i"(offsetof(context_t, fpu.fpr[10])),
				[fr11]"i"(offsetof(context_t, fpu.fpr[11])),
				[fr12]"i"(offsetof(context_t, fpu.fpr[12])),
				[fr13]"i"(offsetof(context_t, fpu.fpr[13])),
				[fr14]"i"(offsetof(context_t, fpu.fpr[14])),
				[fr15]"i"(offsetof(context_t, fpu.fpr[15]))
    );
	asm volatile(
			"lfd %%f16,%[fr16](%%r3)		\n\t"
			"lfd %%f17,%[fr17](%%r3)		\n\t"
			"lfd %%f18,%[fr18](%%r3)		\n\t"
			"lfd %%f19,%[fr19](%%r3)		\n\t"
			"lfd %%f20,%[fr20](%%r3)		\n\t"
			"lfd %%f21,%[fr21](%%r3)		\n\t"
			"lfd %%f22,%[fr22](%%r3)		\n\t"
			"lfd %%f23,%[fr23](%%r3)		\n\t"
			"lfd %%f24,%[fr24](%%r3)		\n\t"
			"lfd %%f25,%[fr25](%%r3)		\n\t"
			"lfd %%f26,%[fr26](%%r3)		\n\t"
			"lfd %%f27,%[fr27](%%r3)		\n\t"
			"lfd %%f28,%[fr28](%%r3)		\n\t"
			"lfd %%f29,%[fr29](%%r3)		\n\t"
			"lfd %%f30,%[fr30](%%r3)		\n\t"
			"lfd %%f31,%[fr31](%%r3)		\n\t"
				::
				[fr16]"i"(offsetof(context_t, fpu.fpr[16])),
				[fr17]"i"(offsetof(context_t, fpu.fpr[17])),
				[fr18]"i"(offsetof(context_t, fpu.fpr[18])),
				[fr19]"i"(offsetof(context_t, fpu.fpr[19])),
				[fr20]"i"(offsetof(context_t, fpu.fpr[20])),
				[fr21]"i"(offsetof(context_t, fpu.fpr[21])),
				[fr22]"i"(offsetof(context_t, fpu.fpr[22])),
				[fr23]"i"(offsetof(context_t, fpu.fpr[23])),
				[fr24]"i"(offsetof(context_t, fpu.fpr[24])),
				[fr25]"i"(offsetof(context_t, fpu.fpr[25])),
				[fr26]"i"(offsetof(context_t, fpu.fpr[26])),
				[fr27]"i"(offsetof(context_t, fpu.fpr[27])),
				[fr28]"i"(offsetof(context_t, fpu.fpr[28])),
				[fr29]"i"(offsetof(context_t, fpu.fpr[29])),
				[fr30]"i"(offsetof(context_t, fpu.fpr[30])),
				[fr31]"i"(offsetof(context_t, fpu.fpr[31]))
	);


    asm volatile(
                 "lwz %%r11,%[gpr11](%%r3)      \n\t"
                 "lwz %%r0,%[srr0](%%r3)        \n\t"
                 "mtsrr0 %%r0                   \n\t"
                 "lwz %%r0,%[srr1](%%r3)        \n\t"
                 "rlwinm %%r0,%%r0,0,14,12      \n\t"
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
