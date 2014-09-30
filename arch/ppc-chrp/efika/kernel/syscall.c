/*
    Copyright © 2008-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <asm/mpc5200b.h>
#include <aros/kernel.h>
#include <utility/tagitem.h>
#include "kernel_intern.h"
#include "syscall.h"

extern char * __text_start;
extern char * __text_end;
extern struct TagItem *BootMsg;

void __attribute__((noreturn)) syscall_handler(regs_t *ctx, uint8_t exception, void *self)
{
    struct KernelBase *KernelBase = getKernelBase();
    int cause = 0;

    //D(bug("[KRN] SysCall handler. context @ %p SC=%d\n", ctx, ctx->gpr[3]));

    if ((char*)ctx->srr0 < &__text_start || (char*)ctx->srr0 >= &__text_end)
    {
        D(bug("[KRN] ERROR ERROR! SysCall issued directly outside kernel.resource!!!!!\n"));
        core_LeaveInterrupt(ctx);
    }

    switch (ctx->gpr[3])
    {
        case SC_CLI:
            ctx->srr1 &= ~MSR_EE;
            break;

        case SC_STI:
            ctx->srr1 |= MSR_EE;
            break;

        case SC_SUPERSTATE:
        	ctx->gpr[3] = ctx->srr1;
            ctx->srr1 &= ~MSR_PR;
            break;

        case SC_ISSUPERSTATE:
            if (ctx->srr1 & MSR_PR)
                ctx->gpr[3] = 0;
            else
                ctx->gpr[3] = 1;
            break;

        case SC_CAUSE:
//            {
//                struct ExecBase *SysBase = getSysBase();
//                if (SysBase)
//                    core_Cause(SysBase);
//            }
        	cause = 1;
            break;

        case SC_DISPATCH:
            core_Dispatch(ctx);
            break;

        case SC_SWITCH:
            core_Switch(ctx);
            break;

        case SC_SCHEDULE:
            core_Schedule(ctx);
            break;

        case SC_RTAS:
        	ctx->gpr[3] = core_Rtas(ctx->gpr[4], ctx->gpr[5], ctx->gpr[6]);
        	break;

        case SC_INVALIDATED:
        {
            char *start = (char*)((IPTR)ctx->gpr[4] & 0xffffffe0);
            char *end = (char*)(((IPTR)ctx->gpr[4] + ctx->gpr[5] + 31) & 0xffffffe0);
            char *ptr;

            for (ptr = start; ptr < end; ptr +=32)
            {
                asm volatile("dcbi 0,%0"::"r"(ptr));
            }
            asm volatile("sync");
            break;
        }

        case SC_REBOOT:
        {
        	uint64_t newtbu = mftbu() + 132000000/4;
        	D(bug("[KRN] REBOOT..."));
        	while(newtbu > mftbu());
        	D(bug("3..."));
        	newtbu = mftbu() + 132000000/4;
        	while(newtbu > mftbu());
        	D(bug("2..."));
        	newtbu = mftbu() + 132000000/4;
        	while(newtbu > mftbu());
        	D(bug("1..."));
        	newtbu = mftbu() + 132000000/4;
        	while(newtbu > mftbu());
        	D(bug("\n\n\n"));

        	void (*restart)(struct TagItem *) = krnGetTagData(KRN_KernelBase, NULL, BootMsg);
        	restart(BootMsg);
        	break;
        }
    }

    if (cause)
    	core_ExitInterrupt(ctx);
    else
    	core_LeaveInterrupt(ctx);
}

static void __attribute__((used)) __rtas_entry_template()
{
	asm volatile(".globl core_Rtas; .type core_Rtas,@function\n"
"core_Rtas:		stwu %r1, -64(%r1)\n"	/* Create proper stack frame */
"				mflr %r0\n"				/* Store the link pointer */
"				stw	 %r0,68(%r1)\n"
"				stw  %r1,8(%r1)\n"		/* Store the stack pointer for virtual space */
"				stw	 %r3,12(%r1)\n"		/* Store the rtas call structure */
"				stw	 %r4,16(%r1)\n"		/* Store the rtas base */
"				stw  %r5,20(%r1)\n"		/* Store the entry address */
"				mfmsr %r0\n"
"				stw	 %r0,24(%r1)\n"
"				lis  %r0,virt2phys@ha\n"	/* VirtualToPhysical function */
"				ori  %r0, %r0, virt2phys@l\n"
"				mtctr %r0\n"
"				stw	 %r0,28(%r1)\n"
"				lis  %r3,1f@ha\n"		/* Get the return point from rtas */
"				ori  %r3, %r3, 1f@l\n"
"				bctrl \n"
"				stw  %r3,32(%r1)\n"
"				mr	 %r3, %r1\n"		/* Convert the stack to physical address */
"				lwz	 %r0,28(%r1)\n"
"				mtctr %r0\n"
"				bctrl \n"
"				mtsprg2 %r3\n"			/* And store it in sprg2 */
"				lwz	 %r3,12(%r1)\n"
"				lwz	 %r4,16(%r1)\n"
"				lwz	 %r0,20(%r1)\n"
"				mtsrr0 %r0\n"
"				lwz	 %r0,32(%r1)\n"
"				mtlr %r0\n"
"				lwz	 %r0,24(%r1)\n"
"				rlwinm	%r0,%r0,0,28,25\n"
"				mtsrr1 %r0\n"
"				sync; isync;\n"
"				rfi\n"
"1:\n"
"				mfsprg2 %r5\n"			/* We're back. Get the stack frame */
"				lwz	 %r4,24(%r5)\n"		/* Old msr */
"				mtsrr1 %r4\n"
"				lwz	 %r4,68(%r5)\n"		/* Old link pointer */
"				mtsrr0 %r4\n"
"				addi %r1, %r1, 64\n"
"				sync; isync; rfi\n"
	);
}
