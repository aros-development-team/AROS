/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
    Full low-level interrupt core. Provides AmigaOS - like preemptive
    multitasking.
*/

/*
    Save/restore macros. These are used to save scratch registers
    while entering interrupt. Inside interrupt if you cover it with
    this macros you may use freely these registers. All other HAS to
    be preserved.
*/

#include "include/machine.i"
#include <asm/segments.h>

#define SAVE_REGS       \
    pushl   %eax;       \
    pushl   %ecx;       \
    pushl   %edx;       \
    push    %ds;        \
    push    %es;        \
    movl    $KERNEL_DS,%edx; \
    mov     %dx,%ds;    \
    mov     %dx,%es

#define RESTORE_REGS    \
    pop     %es;        \
    pop     %ds;        \
    popl    %edx;       \
    popl    %ecx;       \
    popl    %eax

#define R_es        0x00
#define R_ds        0x04
#define R_edx       0x08
#define R_ecx       0x0c
#define R_eax       0x10
#define R_param     0x14

/*
    Save/restore all registers. These macros are used mostly inside Switch
    procedure.
*/

#define SAVE_ALL        \
    push    %es;        \
    push    %ds;        \
    pushl   %eax;       \
    pushl   %ebp;       \
    pushl   %edi;       \
    pushl   %esi;       \
    pushl   %edx;       \
    pushl   %ecx;       \
    pushl   %ebx;       \
    movl    $KERNEL_DS,%edx; \
    mov     %dx,%ds;    \
    mov     %dx,%es

#define RESTORE_ALL     \
    popl    %ebx;       \
    popl    %ecx;       \
    popl    %edx;       \
    popl    %esi;       \
    popl    %edi;       \
    popl    %ebp;       \
    popl    %eax;       \
    pop     %ds;        \
    pop     %es

                    .text

                    .balign 32,0x90
                    .globl  Exec_Permit_Supervisor
                    .type   Exec_Permit_Supervisor,@function

Exec_Permit_Supervisor:
                    testl   $3,4(%esp)      /* Check whether called from user mode */
                    jne     Exec_Schedule   /* Call Schedule() if yes */
                    iret                    /* Exit otherwise */

                    .balign 32,0x90
                    .globl  sys_Supervisor
                    .type   sys_Supervisor,@function

sys_Supervisor:     addl    $4,%esp     /* Remove caller address */
                    RESTORE_REGS        /* Restore all registers */
                    addl    $4,%esp     /* Remove call code */
                    jmp     *%edx       /* Call user code */

                    .balign 32,0x90
                    .globl  exec_SuperState
                    .type   exec_SuperState,@function
exec_SuperState:
	xorl	%eax, %eax				/* clear eax */
	testl	$0x3000,0x08(%esp)	/* check in save EFLAGS if called from SU mode */
	je		exec_SS_IRET			/* if yes we don't need this routine */
	movl	%esp,%eax				/* return int handler stack */
	movl	0x0c(%eax),%esp		/* use user stack */
	pushl	(%eax)					/* push return address */
	ret								/* return after Supervisor() call */

exec_SS_IRET:
	iret

/* for UserState() see userstate.c */

/*
    System call routine. It executes given C code through 'int $0x80' call.
    The function code is specified in %eax register. Functions available are
    as follows:

     0 -> execute Cause() routine
    -1 -> execute ColdReboot() routine
    -2 -> execute Supervisor() routine

    Values not defined here are ignored.

    After executing C function, the ExitIntr is issued. If the interrupt was
    called from user mode and task switching is allowed, Schedule() is started.
*/

                    .text
                    .balign 32,0x90
                    .globl  Exec_SystemCall
                    .type   Exec_systemCall,@function

Exec_SystemCall:    negl    %eax
                    pushl   %eax
                    SAVE_REGS
                    cmpl    $4,%eax
                    jae     restore_all
                    call    *sys_call_table(,%eax,4)
                    movl    %eax,R_eax(%esp)
                    jmp     Exec_ExitIntr
restore_all:        RESTORE_REGS
                    addl    $4,%esp
                    iret

                    .balign 32,0x90
                    .globl  Exec_ExitIntr
                    .type   Exec_ExitIntr,@function

Exec_ExitIntr:      cmpw    $KERNEL_DS,R_ds(%esp)
                    je      restore_all
		    cli
                    movl    4,%eax
		    testw   $0x20,SysFlags(%eax)	/* SFF_SoftInt set? */
		    je	    Exec_ExitIntr_NoSoftInts

		    pushl   %eax
		    call    sys_Cause
		    popl    %eax
		    		    
Exec_ExitIntr_NoSoftInts:
                    cmpb    $0,TDNestCnt(%eax)
                    jge     restore_all
                    testw   $0x80,AttnResched(%eax)
                    je      restore_all
                    jmp     Exec_Schedule_x

                    .balign 32,0x90
                    .globl  Exec_Schedule
                    .type   Exec_Schedule,@function

Exec_Schedule:      pushl   $0
                    SAVE_REGS
Exec_Schedule_x:    cli     	    	    	    	    /* stegerg: added -> CHECKME */
    	    	    movl    4,%edx
                    andw    $0xff7f,AttnResched(%edx)
                    movl    ThisTask(%edx),%eax
                    testb   $TF_EXCEPT,tc_Flags(%eax)
                    jne     1f

                    lea     TaskReady(%edx),%ecx
                    cmpl    lh_TailPred(%ecx),%ecx
                    je      restore_all

                    movl    lh_Head(%ecx),%ecx
                    movb    ln_Pri(%ecx),%cl
                    cmpb    ln_Pri(%eax),%cl
                    jge     1f

#warning TODO: Implement tasks Time Slices

                    testw   $0x2000,SysFlags(%edx)
                    je      restore_all

1:                  pushl   %edx
                    pushl   %eax
                    movb    $TS_READY,tc_State(%eax)
                    lea     TaskReady(%edx),%ecx
                    pushl   %ecx
                    call    *Enqueue(%edx)
                    popl    %eax
                    popl    %eax
                    popl    %edx

                    movl    Switch(%edx),%eax
                    movl    %eax,R_param(%esp)
                    RESTORE_REGS
                    ret

/*
    This routine does a task switch. It sores the context of
    running task and calls dispatcher.

    The context is stored in special place inside Task struncture.
*/

                    .balign 32,0x90
                    .globl  Exec_Switch
                    .type   Exec_Switch,@function
Exec_Switch:        cli                         /* Disable all interrupts */
		    cld
                    SAVE_ALL                    /* Save context */

                    movl    4,%eax              /* get ExecBase->ThisTask */
                    movl    ThisTask(%eax),%eax

                    movl    %esp,%esi           /* Copy the context from stack */
                    movl    tc_ETask(%eax),%edi /* into task->tc_ETask->iet_Context */
                    movl    iet_Context(%edi),%edi
                    movl    $14,%ecx
                    rep
                    movsl

                    movl    $Exec_Restore_Gene,%edi

Exec_Switch_Common: movl    4,%ebp
                    movb    IDNestCnt(%ebp),%al
                    movb    $-1,IDNestCnt(%ebp)

    	    	    cli     	    	    	/* stegerg: was "sti" -> CHECKME */

                    movl    ThisTask(%ebp),%ebx
                    movb    %al,tc_IDNestCnt(%ebx)
                    movl    48(%esp),%eax           /* Get USP */
                    movl    %eax,tc_SPReg(%ebx)

                    testb   $TF_SWITCH,tc_Flags(%ebx)
                    je      Exec_Dispatch_x

                    movl    tc_Switch(%ebx),%eax
                    call    *%eax
                    jmp     Exec_Dispatch_x

                    .balign 32,0x90
                    .globl  Exec_Switch_FPU
                    .type   Exec_Switch_FPU,@function
Exec_Switch_FPU:    cli
		    cld
                    SAVE_ALL
                    movl    4,%eax
                    movl    ThisTask(%eax),%eax

                    movl    %esp,%esi
                    movl    tc_ETask(%eax),%edi
                    movl    iet_Context(%edi),%edi
		    
                    movl    $14,%ecx
                    rep
                    movsl
		    
                    fsave   (%edi)

                    movl    $Exec_Restore_FPU,%edi
                    jmp     Exec_Switch_Common

                    .balign 32,0x90
                    .globl  Exec_Switch_SSE
                    .type   Exec_Switch_SSE,@function
Exec_Switch_SSE:    cli
		    cld
                    SAVE_ALL
                    movl    4,%eax
                    movl    ThisTask(%eax),%eax

                    movl    %esp,%esi
                    movl    tc_ETask(%eax),%edi
                    movl    iet_Context(%edi),%edi
                    movl    $14,%ecx
                    rep
                    movsl

                    fxsave  (%edi)

                    movl    $Exec_Restore_SSE,%edi
                    jmp     Exec_Switch_Common

                    .balign 32,0x90
                    .globl  Exec_Dispatch_SSE
                    .type   Exec_Dispatch_SSE,@function
		    
Exec_Dispatch_SSE:  SAVE_ALL
    	    	    movl    $Exec_Restore_SSE,%edi
                    jmp     Exec_Dispatch_com

                    .balign 32,0x90
                    .globl  Exec_Dispatch_FPU
                    .type   Exec_Dispatch_FPU,@function
		    
Exec_Dispatch_FPU:  SAVE_ALL
    	    	    movl    $Exec_Restore_FPU,%edi
                    jmp     Exec_Dispatch_com
		    
                    .balign 32,0x90
                    .globl  Exec_Dispatch
                    .type   Exec_Dispatch,@function
		    
Exec_Dispatch:	    SAVE_ALL
    	     	    movl    $Exec_Restore_Gene,%edi
Exec_Dispatch_com:  movl    4,%ebp
                    movb    $-1,IDNestCnt(%ebp)
/*                    sti*/

Exec_Dispatch_x:    cli
                    leal    TaskReady(%ebp),%eax
                    movl    lh_Head(%eax),%ecx
                    movl    ln_Succ(%ecx),%edx
                    andl    %edx,%edx
                    jne     1f

                    incl    IdleCount(%ebp)
                    orw     $0x80,AttnResched(%ebp)
                    sti
                    hlt
		    
		    cli
		    testw   $0x20,SysFlags(%ebp) /* SFF_SoftInt set? */
		    je	    Exec_Dispatch_x
		    
		    call    sys_Cause
		    
                    jmp     Exec_Dispatch_x

1:                  movl    %edx,lh_Head(%eax)
                    movl    %eax,ln_Pred(%edx)
                    incl    DispCount(%ebp)
                    movl    %ecx,ThisTask(%ebp)

                    /* Handle Quantum fields!!!! */

		    movw    Quantum(%ebp),%ax
		    movw    %ax,Elapsed(%ebp)

                    andl    $~0x2000,SysFlags(%ebp) /* stegerg: fixme, should be andw!! */
                    movb    $TS_RUN,tc_State(%ecx)
                    movb    tc_IDNestCnt(%ecx),%al
                    movb    %al,IDNestCnt(%ebp)
                    cmpb    $0,IDNestCnt(%ebp)
                    jge     1f
                    /* sti */
1:
                    testb   $TF_EXCEPT | TF_LAUNCH,tc_Flags(%ecx)
                    je      1f
                    call    exec_ProcessFlags
1:                  jmp     *%edi

exec_ProcessFlags:
                    testb   $TF_LAUNCH,tc_Flags(%ecx)
                    je      1f
                    movl    tc_Launch(%ecx),%eax
                    call    *%eax
1:
                    ret     /* HANDLE TASK FALGS!!!! */

Exec_Restore_Gene:  movl    %esp,%edi
                    movl    tc_ETask(%ecx),%esi
                    movl    iet_Context(%esi),%esi
                    movl    $14,%ecx
		    cld
                    rep
                    movsl

                    RESTORE_ALL
                    iret

Exec_Restore_FPU:   movl    %esp,%edi
                    movl    tc_ETask(%ecx),%esi
                    movl    iet_Context(%esi),%esi
                    movl    $14,%ecx
                    rep
                    movsl
                    frstor  (%esi)
                    RESTORE_ALL
                    iret

Exec_Restore_SSE:   movl    %esp,%edi
                    movl    tc_ETask(%ecx),%esi
                    movl    iet_Context(%esi),%esi
                    movl    $14,%ecx
                    rep
                    movsl
                    fxrstor (%esi)
                    RESTORE_ALL
                    iret

