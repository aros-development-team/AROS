/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <hardware/intbits.h>
#include <asm/amcc440.h>
#include <stddef.h>


#include "kernel_base.h"
#include "kernel_intern.h"
#include "kernel_syscall.h"
#include "kernel_globals.h"
#include "kernel_interrupts.h"
#include "kernel_intr.h"

void *__cEXCEPTION_0_Prolog();
void *__mcEXCEPTION_1_Prolog();
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
void *__cEXCEPTION_12_Prolog();
void *__EXCEPTION_13_Prolog();
void *__EXCEPTION_14_Prolog();
void *__cEXCEPTION_15_Prolog();

void intr_init()
{
    D(bug("[KRN] Setting up exception handlers\n"));
    wrspr(IVPR, ((uint32_t)&__cEXCEPTION_0_Prolog) & 0xffff0000);

    wrspr(IVOR0, ((uint32_t)&__cEXCEPTION_0_Prolog) & 0x0000fff0);
    wrspr(IVOR1, ((uint32_t)&__mcEXCEPTION_1_Prolog) & 0x0000fff0);
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
    wrspr(IVOR12, ((uint32_t)&__cEXCEPTION_12_Prolog) & 0x0000fff0);
    wrspr(IVOR13, ((uint32_t)&__EXCEPTION_13_Prolog) & 0x0000fff0);
    wrspr(IVOR14, ((uint32_t)&__EXCEPTION_14_Prolog) & 0x0000fff0);
    wrspr(IVOR15, ((uint32_t)&__cEXCEPTION_15_Prolog) & 0x0000fff0);

    uic_init();
}

#define EXCEPTION_STACK_SIZE    8192
/* Exception Stack
 * Principle of operation:
 *
 *   If an exception occurs from MSR_PR context (user space), switch
 *   the stack to the exception stack.
 *
 *   If an exception occurs from ~MSR_PR context (supervisor), use
 *   the existing stack (whether exception or original supervisor).
 */
ULONG __attribute__((aligned(16))) exception_stack[EXCEPTION_STACK_SIZE / sizeof(ULONG)];

exception_handler * const exception_handlers[16] = {
    generic_handler,            /*  0 - Critical Input   CE */
    generic_handler,            /*  1 - Machine Check    ME */
    generic_handler,            /*  2 - Data Storage     -- */
    generic_handler,            /*  3 - Instr. Storage   -- */
    uic_handler,                /*  4 - External Input   EE */
    alignment_handler,          /*  5 - Alignment        -- */
    program_handler,            /*  6 - Program          -- */
    generic_handler,            /*  7 - FP Unavailable   -- */
    syscall_handler,            /*  8 - System Call      -- */
    generic_handler,            /*  9 - AP Unavailable   -- */
    decrementer_handler,        /* 10 - Decrementer      EE */
    generic_handler,            /* 11 - Fixed Interval   EE */
    generic_handler,            /* 12 - Watchdog         CE */
    mmu_handler,                /* 13 - Data TLB         -- */
    mmu_handler,                /* 14 - Inst TLB         -- */
    generic_handler,            /* 15 - Debug            DE */
};

#if DEBUG

#include <proto/debug.h>
#include <libraries/debug.h>

static CONST_STRPTR symbolfor(struct Library *DebugBase, IPTR addr)
{
    STRPTR modname = "(unknown)";
    STRPTR symname = "(unknown)";
    IPTR offset = 0;
    static TEXT buff[70];
    struct TagItem tags[] = {
        { DL_ModuleName, (IPTR)&modname },
        { DL_SymbolName, (IPTR)&symname },
        { DL_SymbolStart, (IPTR)&offset },
        { TAG_END }
    };
    if (DebugBase) {
        DecodeLocationA((APTR)addr, tags);
        snprintf(buff, sizeof(buff), "%s %s+0x%x", modname, symname, (unsigned)(addr - offset));
        buff[sizeof(buff)-1]=0;
    } else {
        buff[0] = 0;
    }

    return buff;
}
#else
static inline CONST_STRPTR symbolfor(struct Library *DebugBase, IPTR addr) { return ""; }
#endif


void dumpregs(context_t *ctx, uint8_t exception)
{
    uint32_t *sp;
    ULONG *p;
    int i;
    struct Library *DebugBase = (APTR)FindName(&SysBase->LibList, "debug.library");

    bug("[KRN] Exception %d handler. Context @ %p, SysBase @ %p, KernelBase @ %p\n", exception, ctx, SysBase, KernelBase);
    bug("[KRN] SRR0=%08x, SRR1=%08x DEAR=%08x ESR=%08x\n",ctx->cpu.srr0, ctx->cpu.srr1, rdspr(DEAR), rdspr(ESR));
    bug("[KRN] CTR=%08x LR=%08x XER=%08x CCR=%08x\n", ctx->cpu.ctr, ctx->cpu.lr, ctx->cpu.xer, ctx->cpu.ccr);
    bug("[KRN] DAR=%08x DSISR=%08x\n", ctx->cpu.dar, ctx->cpu.dsisr);
    if (exception == 1)
    {
        bug("[KRN] MCSR=%08x\n", rdspr(MCSR));
    }

    for (i = 0; i < 32; i++) {
        if ((i & 3) == 0)
            bug("[KRN]");
        bug(" GPR%02d=%08x", i, ctx->cpu.gpr[i]);
        if ((i & 3) == 3)
            bug("\n");
    }

    bug("[KRN] Instruction dump:");
    p = (ULONG*)ctx->cpu.srr0;
    for (i=0; i < 8; i++)
    {
        if ((i % 4) == 0)
            bug("\n[KRN] %08x:", &p[i]);
        bug(" %08x", p[i]);
    }

    bug("\n[KRN] Stackdump:");
    sp = (uint32_t *)ctx->cpu.gpr[1];
    for (i = 0; i < 64; i++) {
        if ((i % 4) == 0)
            bug("\n[KRN] %08x:", &sp[i]);
        bug(" %08x", sp[i]);
    }

    bug("\n[KRN] Backtrace:  %s\n", symbolfor(DebugBase, ctx->cpu.srr0));
    bug("[KRN] LR=%08x %s\n", ctx->cpu.lr, symbolfor(DebugBase, ctx->cpu.lr));
    if (exception != 13) {
        sp = (uint32_t *)ctx->cpu.gpr[1];
        while(*sp)
        {
                sp = (uint32_t *)sp[0];
                bug("[KRN]    %08x %s\n", sp[1], symbolfor(DebugBase, sp[1]));
        }
    }
    CloseLibrary(DebugBase);

    Debug(0);
}

void handle_exception(context_t *ctx, uint8_t exception)
{
    const ULONG marker = 0x00dead00 | (exception << 24) | exception;

    if (SysBase) {
        struct Task *task = SysBase->ThisTask;
        if (task && (ctx->cpu.srr1 & MSR_PR) && ((APTR)ctx->cpu.gpr[1] <= task->tc_SPLower ||
            (APTR)ctx->cpu.gpr[1] > task->tc_SPUpper)) {
            bug("[KRN]: When did my stack base go from %p to %p?\n",
                    task->tc_SPReg, (APTR)ctx->cpu.gpr[1]);
            dumpregs(ctx, exception);
            for (;;);
        }
    }

    exception_stack[exception] = marker;
    exception_handlers[exception](ctx, exception);
    if ( exception_stack[exception] != marker) {
       bug("[KRN]: Stack overflow processing exception %d\n", exception);
       dumpregs(ctx, exception);
       for (;;);
    }
    DB2(bug("[KRN]: Exception %d: Done\n", exception));

    /* Always disable MSR_POW when exiting */
    ctx->cpu.srr1 &= ~MSR_POW;
}

#define _STR(x) #x
#define STR(x) _STR(x)
#define PUT_INTR_TEMPLATE(num, type) \
    asm volatile(".section .text,\"ax\"\n\t.align 5\n\t.globl __" #type "EXCEPTION_" STR(num) "_Prolog\n\t.type __" #type "EXCEPTION_" STR(num) "_Prolog,@function\n"    \
        "__" #type "EXCEPTION_" STR(num) "_Prolog: \n\t"             \
                  "mtsprg1   %%r3           \n\t" /* SPRG1 = %r3 */  \
                  "mfcr      %%r3           \n\t"                    \
                  "mtsprg2   %%r3           \n\t" /* SPRG2 = %ccr */ \
                  "mfsrr1    %%r3           \n\t" \
                  "andi.     %%r3, %%r3, %[msr_pr] \n\t" \
                  "beq  1f                  \n\t" \
                  "lis       %%r3, exception_stack+%[exc]@ha \n\t" \
                  "la        %%r3, exception_stack+%[exc]@l(%%r3) \n\t" \
                  "b    2f                  \n\t" \
                  "1:                       \n\t" \
                  "addi      %%r3, %%r1, -%[ctx] \n\t" \
                  "2:                       \n\t" \
                  "stw       %%r0, %[gpr0](%%r3)  \n\t" \
                  "stw       %%r1, %[gpr1](%%r3)  \n\t" \
                  "stw       %%r2, %[gpr2](%%r3)  \n\t" \
                  "mfsprg2   %%r0           \n\t" \
                  "stw       %%r0, %[ccr](%%r3)   \n\t" \
                  "mfsprg1   %%r0           \n\t" \
                  "stw       %%r0, %[gpr3](%%r3)  \n\t" \
                  "mr        %%r1, %%r3     \n\t" \
                 :: \
                  [gpr0]"i"(offsetof(context_t, cpu.gpr[0])), \
                  [gpr1]"i"(offsetof(context_t, cpu.gpr[1])), \
                  [gpr2]"i"(offsetof(context_t, cpu.gpr[2])), \
                  [gpr3]"i"(offsetof(context_t, cpu.gpr[3])), \
                  [ccr]"i"(offsetof(context_t, cpu.ccr)), \
                  [ctx]"i"(sizeof(context_t)), \
                  [exc]"i"(sizeof(exception_stack) - sizeof(context_t)), \
                  [msr_pr]"i"(MSR_PR)); \
        asm volatile (\
                  "mf" #type "srr0 %%r0              \n\t" \
                  "stw       %%r0,%[srr0](%%r3)   \n\t" \
                  "mf" #type "srr1 %%r0              \n\t" \
                  "stw       %%r0,%[srr1](%%r3)   \n\t" \
                  "mfctr     %%r0               \n\t" \
                  "stw       %%r0,%[ctr](%%r3)    \n\t" \
                  "mflr      %%r0                \n\t" \
                  "stw       %%r0,%[lr](%%r3)     \n\t" \
                  "mfxer     %%r0               \n\t" \
                  "stw       %%r0,%[xer](%%r3)    \n\t" \
                  "stw       %%r4, %[gpr4](%%r3)  \n\t" \
                  "stw       %%r5, %[gpr5](%%r3)  \n\t" \
                  "li        %%r4, %[irq]          \n\t" \
                  "bl        __EXCEPTION_Trampoline\n\t" \
                  "lwz       %%r5, %[gpr5](%%r3)  \n\t" \
                  "lwz       %%r4, %[gpr4](%%r3)  \n\t" \
                  "addi      %%r0, %%r3, -4    \n\t" /* Dummy write */    \
                  "stwcx.    %%r0, 0, %%r0     \n\t" /* to clear resv. */ \
                  "lwz       %%r0, %[xer](%%r3)   \n\t" \
                  "mtxer     %%r0               \n\t" \
                  "lwz       %%r0, %[lr](%%r3)    \n\t" \
                  "mtlr      %%r0                \n\t" \
                  "lwz       %%r0, %[ctr](%%r3)   \n\t" \
                  "mtctr     %%r0               \n\t" \
                  "lwz       %%r0,%[srr1](%%r3)   \n\t" \
                  "mt" #type "srr1 %%r0              \n\t" \
                  "lwz       %%r0,%[srr0](%%r3)   \n\t" \
                  "mt" #type "srr0 %%r0              \n\t" \
                  "lwz       %%r0, %[ccr](%%r3)   \n\t" \
                  "mtcr      %%r0                \n\t" \
                  "lwz       %%r0, %[gpr0](%%r3)  \n\t" \
                  "lwz       %%r1, %[gpr1](%%r3)  \n\t" \
                  "lwz       %%r2, %[gpr2](%%r3)  \n\t" \
                  "lwz       %%r3, %[gpr3](%%r3)  \n\t" \
                  "sync; isync; rf" #type "i \n\t" \
                  \
                  :: \
                  [gpr0]"i"(offsetof(context_t, cpu.gpr[0])), \
                  [gpr1]"i"(offsetof(context_t, cpu.gpr[1])), \
                  [gpr2]"i"(offsetof(context_t, cpu.gpr[2])), \
                  [gpr3]"i"(offsetof(context_t, cpu.gpr[3])), \
                  [gpr4]"i"(offsetof(context_t, cpu.gpr[4])), \
                  [gpr5]"i"(offsetof(context_t, cpu.gpr[5])), \
                  [ccr]"i"(offsetof(context_t, cpu.ccr)), \
                  [srr0]"i"(offsetof(context_t, cpu.srr0)), \
                  [srr1]"i"(offsetof(context_t, cpu.srr1)), \
                  [ctr]"i"(offsetof(context_t, cpu.ctr)), \
                  [lr]"i"(offsetof(context_t, cpu.lr)), \
                  [xer]"i"(offsetof(context_t, cpu.xer)), \
                  [irq]"i"(num) \
                  ); \

uint64_t idle_time;
static uint64_t last_calc;

void decrementer_handler(context_t *ctx, uint8_t exception)
{
    /* Clear the DIS bit - we have received decrementer exception */
    wrspr(TSR, TSR_DIS);
    DB2(bug("[KRN] Decrementer handler. Context @ %p. srr1=%08x\n", ctx, ctx->cpu.srr1));

    if (!KernelBase)
        return;

    /* Idle time calculator */

    uint64_t current = mftbu();
    if (current - last_calc > KernelBase->kb_PlatformData->pd_OPBFreq)
    {
    	uint32_t total_time = current - last_calc;

    	if (total_time < idle_time)
    		total_time = idle_time;

    	KernelBase->kb_PlatformData->pd_CPUUsage = 1000 - ((uint32_t)idle_time) / (total_time / 1000);

    	if (KernelBase->kb_PlatformData->pd_CPUUsage > 999)
    	{
    		DB2(bug("[KRN] CPU usage: %3d.%d (%s)\n", KernelBase->kb_PlatformData->pd_CPUUsage / 10, KernelBase->kb_PlatformData->pd_CPUUsage % 10,
    				SysBase->ThisTask->tc_Node.ln_Name));
    	}
    	else
    		DB2(bug("[KRN] CPU usage: %3d.%d\n", KernelBase->kb_PlatformData->pd_CPUUsage / 10, KernelBase->kb_PlatformData->pd_CPUUsage % 10));

    	idle_time = 0;
    	last_calc = current;
    }

    /* Signal the Exec VBlankServer */
    if (SysBase && (SysBase->IDNestCnt < 0)) {
        core_Cause(INTB_VERTB, 1L << INTB_VERTB);
    }

    ExitInterrupt(ctx);
}

void generic_handler(context_t *ctx, uint8_t exception)
{
    struct KernelBase *KernelBase = getKernelBase();

    DB2(bug("[KRN] Generic handler. Context @ %p. srr1=%08x\n", ctx, ctx->cpu.srr1));

    if (!krnRunExceptionHandlers(KernelBase, exception, ctx))
    {
        D(dumpregs(ctx, exception));
        D(bug("[KRN] **UNHANDLED EXCEPTION** stopping here...\n"));

        while(1) {
            wrmsr(rdmsr() | MSR_POW);
        }
    }

    ExitInterrupt(ctx);
}

void program_handler(context_t *ctx, uint8_t exception)
{

    uint32_t pc;
    BOOL exchandled = FALSE;

    DB2(bug("[KRN] Program handler. Context @ %p. srr1=%08x\n", ctx, ctx->cpu.srr1));

    if(rdspr(ESR) & ESR_PIL)
    {
        pc = ctx->cpu.srr0;

        DB2(bug("[KRN] Illegal Instruction @ %08x\n", pc));

        /* Check if lwsync is executed */
        if (*(uint32_t *)pc == 0x7c2004ac)
        {
            bug("[KRN] Warning: emulating 'lwsync'\n");

            exchandled = TRUE;

            /* PPC440 doesn't have lwsync, do sync instead */
            asm volatile("sync" : : : "memory");

            /* move the return address forward so the
                  instruction isn't run again */
            ctx->cpu.srr0 += 4;
        }
    }

    if (!exchandled)
        generic_handler(ctx, exception);

    ExitInterrupt(ctx);
}

void  mmu_handler(context_t *ctx, uint8_t exception)
{
    if (!!krnRunExceptionHandlers(KernelBase, exception, ctx))
    {
        /* Any unhandled MMU activity is fatal for now. */
        dumpregs(ctx, exception);
    }
}

double lfd(intptr_t addr)
{
	union {
		uint8_t		u8[8];
		uint16_t	u16[4];
		uint32_t	u32[2];
		uint64_t	u64;
		float		f[2];
		double		d;
	} conv;

	switch ((intptr_t)addr & 3)
	{
	case 0:
		conv.u32[0] = ((uint32_t *)addr)[0];
		conv.u32[1] = ((uint32_t *)addr)[1];
		break;

	case 2:
		conv.u16[0] = ((uint16_t *)addr)[0];
		conv.u16[1] = ((uint16_t *)addr)[1];
		conv.u16[2] = ((uint16_t *)addr)[2];
		conv.u16[3] = ((uint16_t *)addr)[3];
		break;

	default:
		conv.u8[0] = ((uint8_t *)addr)[0];
		conv.u8[1] = ((uint8_t *)addr)[1];
		conv.u8[2] = ((uint8_t *)addr)[2];
		conv.u8[3] = ((uint8_t *)addr)[3];
		conv.u8[4] = ((uint8_t *)addr)[4];
		conv.u8[5] = ((uint8_t *)addr)[5];
		conv.u8[6] = ((uint8_t *)addr)[6];
		conv.u8[7] = ((uint8_t *)addr)[7];
		break;
	}

	return conv.d;
}

float lfs(intptr_t addr)
{
	union {
		uint8_t		u8[8];
		uint16_t	u16[4];
		uint32_t	u32[2];
		uint64_t	u64;
		float		f[2];
		double		d;
	} conv;

	switch ((intptr_t)addr & 3)
	{
	case 0:
		conv.u32[0] = ((uint32_t *)addr)[0];
		break;

	case 2:
		conv.u16[0] = ((uint16_t *)addr)[0];
		conv.u16[1] = ((uint16_t *)addr)[1];
		break;

	default:
		conv.u8[0] = ((uint8_t *)addr)[0];
		conv.u8[1] = ((uint8_t *)addr)[1];
		conv.u8[2] = ((uint8_t *)addr)[2];
		conv.u8[3] = ((uint8_t *)addr)[3];
		break;
	}

	return conv.f[0];
}

void stfd(double v, intptr_t addr)
{
	union {
		uint8_t		u8[8];
		uint16_t	u16[4];
		uint32_t	u32[2];
		uint64_t	u64;
		float		f[2];
		double		d;
	} conv;

	conv.d = v;

	switch ((intptr_t)addr & 3)
	{
	case 0:
		((uint32_t *)addr)[0] = conv.u32[0];
		((uint32_t *)addr)[1] = conv.u32[1];
		break;

	case 2:
		((uint16_t *)addr)[0] = conv.u16[0];
		((uint16_t *)addr)[1] = conv.u16[1];
		((uint16_t *)addr)[2] = conv.u16[2];
		((uint16_t *)addr)[3] = conv.u16[3];
		break;

	default:
		((uint8_t *)addr)[0] = conv.u8[0];
		((uint8_t *)addr)[1] = conv.u8[1];
		((uint8_t *)addr)[2] = conv.u8[2];
		((uint8_t *)addr)[3] = conv.u8[3];
		((uint8_t *)addr)[4] = conv.u8[4];
		((uint8_t *)addr)[5] = conv.u8[5];
		((uint8_t *)addr)[6] = conv.u8[6];
		((uint8_t *)addr)[7] = conv.u8[7];
		break;
	}
}

void stfs(float v, intptr_t addr)
{
	union {
		uint8_t		u8[8];
		uint16_t	u16[4];
		uint32_t	u32[2];
		uint64_t	u64;
		float		f[2];
		double		d;
	} conv;

	conv.f[0] = v;

	switch ((intptr_t)addr & 3)
	{
	case 0:
		((uint32_t *)addr)[0] = conv.u32[0];
		break;

	case 2:
		((uint16_t *)addr)[0] = conv.u16[0];
		((uint16_t *)addr)[1] = conv.u16[1];
		break;

	default:
		((uint8_t *)addr)[0] = conv.u8[0];
		((uint8_t *)addr)[1] = conv.u8[1];
		((uint8_t *)addr)[2] = conv.u8[2];
		((uint8_t *)addr)[3] = conv.u8[3];
		break;
	}
}

void alignment_handler(context_t *ctx, uint8_t exception)
{
    int fixed = 1;

    intptr_t dear = rdspr(DEAR);
    uint32_t insn = *(uint32_t *)ctx->cpu.srr0;

    uint8_t reg = (insn >> 21) & 0x1f;		// source/dest register
    uint8_t areg = (insn >> 16) & 0x1f;		// register to be updated with dear value

    D(bug("[KRN] Alignment handler. Context @ %p. srr1=%08x\n", ctx, ctx->cpu.srr1));


    switch (insn >> 26)
    {
    case 50: 	// lfd
    	ctx->fpu.fpr[reg] = lfd(dear);
    	break;
    case 51:	// lfdu
    	ctx->fpu.fpr[reg] = lfd(dear);
    	ctx->cpu.gpr[areg] = dear;
    	break;
    case 48:	// lfs
    	ctx->fpu.fpr[reg] = lfs(dear);
    	break;
    case 49:	// lfsu
    	ctx->fpu.fpr[reg] = lfs(dear);
    	ctx->cpu.gpr[areg] = dear;
    	break;
    case 54:	// stfd
    	stfd(ctx->fpu.fpr[reg], dear);
    	break;
    case 55:	// stfdu
		stfd(ctx->fpu.fpr[reg], dear);
    	ctx->cpu.gpr[areg] = dear;
    	break;
    case 52:	// stfs
    	stfs(ctx->fpu.fpr[reg], dear);
    	break;
    case 53:	// stfsu
    	stfs(ctx->fpu.fpr[reg], dear);
    	ctx->cpu.gpr[areg] = dear;
    	break;
    case 31:	// lfdux, lfdx, lfsux, lfsx, stfdux, stfdx,  stfsux, stfsx
    	switch ((insn & 0x00001ffe) >> 1)
    	{
    	case 631: // lfdux
    		ctx->fpu.fpr[reg] = lfd(dear);
    		ctx->cpu.gpr[areg] = dear;
    		break;
    	case 599: // lfdx
    		ctx->fpu.fpr[reg] = lfd(dear);
    		break;
    	case 567: // lfsux
    		ctx->fpu.fpr[reg] = lfs(dear);
    		ctx->cpu.gpr[areg] = dear;
    		break;
    	case 535: // lfsx
    		ctx->fpu.fpr[reg] = lfs(dear);
    		break;
    	case 759: // stfdux
    		stfd(ctx->fpu.fpr[reg], dear);
    		ctx->cpu.gpr[areg] = dear;
    		break;
    	case 727: // stfdx
    		stfd(ctx->fpu.fpr[reg], dear);
    		break;
    	case 695: // stfsux
    		stfs(ctx->fpu.fpr[reg], dear);
    		ctx->cpu.gpr[areg] = dear;
    		break;
    	case 663: // stfsx
    		stfs(ctx->fpu.fpr[reg], dear);
    		break;
    	default:
    		fixed = 0;
    		break;
    	}
    	break;
    default:
    	fixed = 0;
    	break;
    }

    if (fixed)
    {
    	ctx->cpu.srr0 += 4;
    	return;
    }
    else
    {
        D(bug("[KRN] Alignment exception handler failed to help... INSN=%08x, DEAR=%08x\n", insn, dear));
    	generic_handler(ctx, exception);
    }
}


static void __attribute__((used)) __EXCEPTION_Prolog_template()
{

    /*
     * Create partial context on the stack. It is impossible to save it fully since the
     * exception handlers have limited size. This code will do as much as possible and then
     * jump to general trampoline...
     */

    PUT_INTR_TEMPLATE(0,c);    /* crit */
    PUT_INTR_TEMPLATE(1,mc);   /* machine check */
    PUT_INTR_TEMPLATE(2,);
    PUT_INTR_TEMPLATE(3,);
    PUT_INTR_TEMPLATE(4,);
    PUT_INTR_TEMPLATE(5,);
    PUT_INTR_TEMPLATE(6,);
    PUT_INTR_TEMPLATE(7,);
    PUT_INTR_TEMPLATE(8,);
    PUT_INTR_TEMPLATE(9,);
    PUT_INTR_TEMPLATE(10,);
    PUT_INTR_TEMPLATE(11,);
    PUT_INTR_TEMPLATE(12, c);  /* crit */
    PUT_INTR_TEMPLATE(13,);
    PUT_INTR_TEMPLATE(14,);
    PUT_INTR_TEMPLATE(15, c);  /* crit */
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
			::
			[gpr6]"i"(offsetof(context_t, cpu.gpr[6])),
			[gpr7]"i"(offsetof(context_t, cpu.gpr[7])),
			[gpr8]"i"(offsetof(context_t, cpu.gpr[8])),
			[gpr9]"i"(offsetof(context_t, cpu.gpr[9])),
			[gpr10]"i"(offsetof(context_t, cpu.gpr[10])),
			[gpr11]"i"(offsetof(context_t, cpu.gpr[11])),
			[gpr12]"i"(offsetof(context_t, cpu.gpr[12])),
			[gpr13]"i"(offsetof(context_t, cpu.gpr[13])),
			[gpr14]"i"(offsetof(context_t, cpu.gpr[14])),
			[gpr15]"i"(offsetof(context_t, cpu.gpr[15])),
			[gpr16]"i"(offsetof(context_t, cpu.gpr[16])),
			[gpr17]"i"(offsetof(context_t, cpu.gpr[17])),
			[gpr18]"i"(offsetof(context_t, cpu.gpr[18])),
			[gpr19]"i"(offsetof(context_t, cpu.gpr[19])),
			[gpr20]"i"(offsetof(context_t, cpu.gpr[20])),
			[gpr21]"i"(offsetof(context_t, cpu.gpr[21])),
			[gpr22]"i"(offsetof(context_t, cpu.gpr[22])),
			[gpr23]"i"(offsetof(context_t, cpu.gpr[23])),
			[gpr24]"i"(offsetof(context_t, cpu.gpr[24])),
			[gpr25]"i"(offsetof(context_t, cpu.gpr[25])),
			[gpr26]"i"(offsetof(context_t, cpu.gpr[26])),
			[gpr27]"i"(offsetof(context_t, cpu.gpr[27])),
			[gpr28]"i"(offsetof(context_t, cpu.gpr[28])),
			[gpr29]"i"(offsetof(context_t, cpu.gpr[29])),
			[gpr30]"i"(offsetof(context_t, cpu.gpr[30])),
			[gpr31]"i"(offsetof(context_t, cpu.gpr[31]))
	);

	asm volatile(
			"mfmsr %%r0                                             \n\t"
			"ori %%r0,%%r0, %[msrval]@l \n\t"
			"mtmsr %%r0; isync                              \n\t"
			"stfd %%f0,%[fr0](%%r3)         \n\t"
			"mffs %%f0                                              \n\t"
			"stfd %%f0,%[fpscr](%%r3)               \n\t"
			"stfd %%f1,%[fr1](%%r3)         \n\t"
			"stfd %%f2,%[fr2](%%r3)         \n\t"
			"stfd %%f3,%[fr3](%%r3)         \n\t"
			"stfd %%f4,%[fr4](%%r3)         \n\t"
			"stfd %%f5,%[fr5](%%r3)         \n\t"
			"stfd %%f6,%[fr6](%%r3)         \n\t"
			"stfd %%f7,%[fr7](%%r3)         \n\t"
			"stfd %%f8,%[fr8](%%r3)         \n\t"
			"stfd %%f9,%[fr9](%%r3)         \n\t"
			"stfd %%f10,%[fr10](%%r3)               \n\t"
			"stfd %%f11,%[fr11](%%r3)               \n\t"
			"stfd %%f12,%[fr12](%%r3)               \n\t"
			"stfd %%f13,%[fr13](%%r3)               \n\t"
			"stfd %%f14,%[fr14](%%r3)               \n\t"
			"stfd %%f15,%[fr15](%%r3)               \n\t"
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
			"stfd %%f16,%[fr16](%%r3)               \n\t"
			"stfd %%f17,%[fr17](%%r3)               \n\t"
			"stfd %%f18,%[fr18](%%r3)               \n\t"
			"stfd %%f19,%[fr19](%%r3)               \n\t"
			"stfd %%f20,%[fr20](%%r3)               \n\t"
			"stfd %%f21,%[fr21](%%r3)               \n\t"
			"stfd %%f22,%[fr22](%%r3)               \n\t"
			"stfd %%f23,%[fr23](%%r3)               \n\t"
			"stfd %%f24,%[fr24](%%r3)               \n\t"
			"stfd %%f25,%[fr25](%%r3)               \n\t"
			"stfd %%f26,%[fr26](%%r3)               \n\t"
			"stfd %%f27,%[fr27](%%r3)               \n\t"
			"stfd %%f28,%[fr28](%%r3)               \n\t"
			"stfd %%f29,%[fr29](%%r3)               \n\t"
			"stfd %%f30,%[fr30](%%r3)               \n\t"
			"stfd %%f31,%[fr31](%%r3)               \n\t"
			"mflr %%r30                             \n\t"
			"mr   %%r31, %%r3                       \n\t"
			"addi %%r1, %%r1, -16                   \n\t"
			"bl handle_exception                    \n\t"
			"addi %%r1, %%r1, 16                    \n\t"
			"mr   %%r3, %%r31                       \n\t"
			"mtlr %%r30                             \n\t"
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
			[gpr12]"i"(offsetof(context_t, cpu.gpr[12])),
			[gpr13]"i"(offsetof(context_t, cpu.gpr[13])),
			[gpr14]"i"(offsetof(context_t, cpu.gpr[14])),
			[gpr15]"i"(offsetof(context_t, cpu.gpr[15])),
			[gpr16]"i"(offsetof(context_t, cpu.gpr[16])),
			[gpr17]"i"(offsetof(context_t, cpu.gpr[17])),
			[gpr18]"i"(offsetof(context_t, cpu.gpr[18])),
			[gpr19]"i"(offsetof(context_t, cpu.gpr[19])),
			[gpr20]"i"(offsetof(context_t, cpu.gpr[20])),
			[gpr21]"i"(offsetof(context_t, cpu.gpr[21])),
			[gpr22]"i"(offsetof(context_t, cpu.gpr[22])),
			[gpr23]"i"(offsetof(context_t, cpu.gpr[23])),
			[gpr24]"i"(offsetof(context_t, cpu.gpr[24])),
			[gpr25]"i"(offsetof(context_t, cpu.gpr[25])),
			[gpr26]"i"(offsetof(context_t, cpu.gpr[26])),
			[gpr27]"i"(offsetof(context_t, cpu.gpr[27])),
			[gpr28]"i"(offsetof(context_t, cpu.gpr[28])),
			[gpr29]"i"(offsetof(context_t, cpu.gpr[29])),
			[gpr30]"i"(offsetof(context_t, cpu.gpr[30])),
			[gpr31]"i"(offsetof(context_t, cpu.gpr[31]))
	);

	asm volatile(
			"lfd  %%f0,%[fpscr](%%r3)               \n\t"
			"mtfsf 255,%%f0                                         \n\t"
			"lfd %%f0,%[fr0](%%r3)          \n\t"
			"lfd %%f1,%[fr1](%%r3)          \n\t"
			"lfd %%f2,%[fr2](%%r3)          \n\t"
			"lfd %%f3,%[fr3](%%r3)          \n\t"
			"lfd %%f4,%[fr4](%%r3)          \n\t"
			"lfd %%f5,%[fr5](%%r3)          \n\t"
			"lfd %%f6,%[fr6](%%r3)          \n\t"
			"lfd %%f7,%[fr7](%%r3)          \n\t"
			"lfd %%f8,%[fr8](%%r3)          \n\t"
			"lfd %%f9,%[fr9](%%r3)          \n\t"
			"lfd %%f10,%[fr10](%%r3)                \n\t"
			"lfd %%f11,%[fr11](%%r3)                \n\t"
			"lfd %%f12,%[fr12](%%r3)                \n\t"
			"lfd %%f13,%[fr13](%%r3)                \n\t"
			"lfd %%f14,%[fr14](%%r3)                \n\t"
			"lfd %%f15,%[fr15](%%r3)                \n\t"
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
			"lfd %%f16,%[fr16](%%r3)                \n\t"
			"lfd %%f17,%[fr17](%%r3)                \n\t"
			"lfd %%f18,%[fr18](%%r3)                \n\t"
			"lfd %%f19,%[fr19](%%r3)                \n\t"
			"lfd %%f20,%[fr20](%%r3)                \n\t"
			"lfd %%f21,%[fr21](%%r3)                \n\t"
			"lfd %%f22,%[fr22](%%r3)                \n\t"
			"lfd %%f23,%[fr23](%%r3)                \n\t"
			"lfd %%f24,%[fr24](%%r3)                \n\t"
			"lfd %%f25,%[fr25](%%r3)                \n\t"
			"lfd %%f26,%[fr26](%%r3)                \n\t"
			"lfd %%f27,%[fr27](%%r3)                \n\t"
			"lfd %%f28,%[fr28](%%r3)                \n\t"
			"lfd %%f29,%[fr29](%%r3)                \n\t"
			"lfd %%f30,%[fr30](%%r3)                \n\t"
			"lfd %%f31,%[fr31](%%r3)                \n\t"
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
			"lwz %%r10,%[gpr10](%%r3)      \n\t"
			"lwz %%r9,%[gpr9](%%r3)        \n\t"
			"lwz %%r8,%[gpr8](%%r3)        \n\t"
			"lwz %%r7,%[gpr7](%%r3)        \n\t"
			"lwz %%r6,%[gpr6](%%r3)        \n\t"
			"blr\n"
			::
			[gpr6]"i"(offsetof(context_t, cpu.gpr[6])),
			[gpr7]"i"(offsetof(context_t, cpu.gpr[7])),
			[gpr8]"i"(offsetof(context_t, cpu.gpr[8])),
			[gpr9]"i"(offsetof(context_t, cpu.gpr[9])),
			[gpr10]"i"(offsetof(context_t, cpu.gpr[10])),
			[gpr11]"i"(offsetof(context_t, cpu.gpr[11]))
	);
}
