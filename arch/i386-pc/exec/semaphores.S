/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$
    
    Desc: Asm callers to functions
    Lang: English
*/

#include "machine.i"

.text

#define NUM_REGS 8
#define FIRST_ARG	((NUM_REGS+1)*4)
#define SECOND_ARG	(FIRST_ARG+4)

#define STUB_ARG0(name)			  \
	pushal				; \
	call	name			; \
	popal				; \
	ret

#define STUB_ARG1(name)			  \
	pushal				; \
	movl	FIRST_ARG(%esp),%eax	; \
	pushl	%eax			; \
	call	name			; \
	addl	$4,%esp			; \
	popal				; \
	ret

#define STUB_ARG2(name)			  \
	pushal				; \
	movl	SECOND_ARG(%esp),%eax	; \
	pushl	%eax			; \
	movl	SECOND_ARG(%esp),%eax	; \
	pushl	%eax			; \
	call	name			; \
	addl	$8,%esp			; \
	popal				; \
	ret
	
#define STUB0(cname,name)		  \
	.globl	cname			; \
	.type	cname,@function		; \
cname:					; \
	STUB_ARG0(name)

#define STUB1(cname,name)		  \
	.globl	cname			; \
	.type	cname,@function		; \
cname:					; \
	STUB_ARG1(name)

#define STUB2(cname,name)		  \
	.globl	cname			; \
	.type	cname,@function		; \
cname:					; \
	STUB_ARG2(name)

	STUB2(AROS_SLIB_ENTRY(ObtainSemaphore,Exec),AROS_CSYMNAME(_Exec_ObtainSemaphore))
	STUB2(AROS_SLIB_ENTRY(ReleaseSemaphore,Exec),AROS_CSYMNAME(_Exec_ReleaseSemaphore))
	STUB2(AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec),AROS_CSYMNAME(_Exec_ObtainSemaphoreShared))

