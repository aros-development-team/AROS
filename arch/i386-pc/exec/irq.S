/*
    Copyright (C) 1997-1998 AROS - The Amiga Research OS
    $Id$

    Desc: The interrupt handler set.
    Lang: english
*/

/*****************************************************************************

    NAME

    SYNOPSIS

    FUNCTION
	Here are some functions to provide normal int handling under
	IBMPC.

    INPUTS

    RESULT
	Working interrupts.

    NOTES

    EXAMPLE

    BUGS
	The CPU exceptions are still missing.
	
    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include "machine.i"
#include "supervisor.i"
#include "intserv.i"

			.text
			.balign	16

/* These are two functions: main_irq and slave_irq. Both sends proper informa-
tions to hardware - EOI signals. Both call then IntServer routine with proper
parameter. After all this things they restore CPU state in user mode */

slave_irq:		outb	%al,$0xa0
			movb	$0x62,%al
main_irq:		outb	%al,$0x20
			movl	(4),%ebx
			movl	iv_Data(%edx,%ebx),%eax
			pushl	%eax
			movl	iv_Code(%edx,%ebx),%eax
			call	*%eax
			lea	4(%esp),%esp
			REG_RESTR
			USER
			iret

/* This are several int starters. When an interrupt occurs, these functions
are executed by CPU. Please DON'T change them, unles you are sure about your
modifications. */

Irq_0:			SUPER
			REG_STORE
			movb	$0x60,%al
			movl	$IVIRQ0,%edx
			jmp	main_irq

Irq_1:			SUPER
			REG_STORE
			movb	$0x61,%al
			movl	$IVIRQ1,%edx
			jmp	main_irq

Irq_2:			SUPER
			REG_STORE
			movb	$0x62,%al
			movl	$IVIRQ2,%edx
			jmp	main_irq

Irq_3:			SUPER
			REG_STORE
			movb	$0x63,%al
			movl	$IVIRQ3,%edx
			jmp	main_irq

Irq_4:			SUPER
			REG_STORE
			movb	$0x64,%al
			movl	$IVIRQ4,%edx
			jmp	main_irq

Irq_5:			SUPER
			REG_STORE
			movb	$0x65,%al
			movl	$IVIRQ5,%edx
			jmp	main_irq

Irq_6:			SUPER
			REG_STORE
			movb	$0x66,%al
			movl	$IVIRQ6,%edx
			jmp	main_irq

Irq_7:			SUPER
			REG_STORE
			movb	$0x67,%al
			movl	$IVIRQ7,%edx
			jmp	main_irq

Irq_8:			SUPER
			REG_STORE
			movb	$0x60,%al
			movl	$IVIRQ8,%edx
			jmp	slave_irq

Irq_9:			SUPER
			REG_STORE
			movb	$0x61,%al
			movl	$IVIRQ9,%edx
			jmp	slave_irq

Irq_a:			SUPER
			REG_STORE
			movb	$0x62,%al
			movl	$IVIRQA,%edx
			jmp	slave_irq

Irq_b:			SUPER
			REG_STORE
			movb	$0x63,%al
			movl	$IVIRQB,%edx
			jmp	slave_irq

Irq_c:			SUPER
			REG_STORE
			movb	$0x64,%al
			movl	$IVIRQC,%edx
			jmp	slave_irq

Irq_d:			SUPER
			REG_STORE
			movb	$0x65,%al
			movl	$IVIRQD,%edx
			jmp	slave_irq

Irq_e:			SUPER
			REG_STORE
			movb	$0x66,%al
			movl	$IVIRQE,%edx
			jmp	slave_irq

Irq_f:			SUPER
			REG_STORE
			movb	$0x67,%al
			movl	$IVIRQF,%edx
			jmp	slave_irq


/* MakeInt routine. This pice of code writes temporary
int table to IDT table. You should call this thing while
system initialisation */

			.globl	MakeInt
			.type	MakeInt,@function
MakeInt:		pushl	%esi
			movl	$inttab,%esi
			pushl	%edi
			pushl	%ecx

			sidt	.idt

			movl	.aptr,%edi	/*start with vector 0x20, base 8 */
			addl	$0x0100,%edi
			movl	$16,%ecx
.mi_l1:			movl	(%esi),%eax
			lea	4(%esi),%esi
			movw	%ax,(%edi)
			rorl	$16,%eax
			movw	%ax,6(%edi)
			lea	8(%edi),%edi
			decl	%ecx
			jne	.mi_l1
			
			popl	%ecx
			popl	%edi
			popl	%esi
			ret

.idt:	.word	0
.aptr:	.long	0

/* Interrupt temporary jump table. */

inttab:	.long	Irq_0
	.long	Irq_1
	.long	Irq_2
	.long	Irq_3
	.long	Irq_4
	.long	Irq_5
	.long	Irq_6
	.long	Irq_7
	.long	Irq_8
	.long	Irq_9
	.long	Irq_a
	.long	Irq_b
	.long	Irq_c
	.long	Irq_d
	.long	Irq_e
	.long	Irq_f
