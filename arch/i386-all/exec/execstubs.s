/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Stubs to call C functions while preserving all registers
    Lang: english
*/
	#include "aros/i386/asm.h"

/*
    Macros:
	PUSH - Save all registers on the stack
	POP - Restore all registers from the stack

	STUB_ARG0(name) - Call a function preserving all registers
		    which gets no arguments
	STUB_ARG1(name) - Call a function preserving all registers
		    which gets a single argument
	STUB_ARG2(name) - Call a function preserving all registers
		    which gets two arguments
*/


#define NUM_REGS    3
#define FIRST_ARG   ((NUM_REGS+2)*4)
#define SECOND_ARG  (FIRST_ARG+4)

#define PUSH			      \
	pushl %eax		    ; \
	pushl %ecx		    ; \
	pushl %edx		    ; \

#define POP			      \
	popl  %edx		    ; \
	popl  %ecx		    ; \
	popl  %eax

#define STUB_ARG0(name)               \
	pushl %ebp		    ; \
	movl  %esp,%ebp		    ; \
	PUSH			    ; \
	call  name		    ; \
	POP			    ; \
	leave                       ; \
	ret

#define STUB_ARG1(name)               \
	pushl %ebp		    ; \
	movl  %esp,%ebp		    ; \
	PUSH			    ; \
	movl  FIRST_ARG(%esp),%edx  ; \
	pushl %edx		    ; \
	call  name		    ; \
	addl  $4,%esp		    ; \
	POP			    ; \
	leave			    ; \
	ret

#define STUB_ARG2(name)               \
	pushl %ebp		    ; \
	movl  %esp,%ebp		    ; \
	PUSH			    ; \
	movl  SECOND_ARG(%esp),%edx ; \
	pushl %edx		    ; \
	movl  SECOND_ARG(%esp),%edx ; \
	pushl %edx		    ; \
	call  name		    ; \
	addl  $8,%esp		    ; \
	POP			    ; \
	leave			    ; \
	ret

/* To save typing work */
#define STUB0(cname,name)             \
	.globl	cname		    ; \
	_FUNCTION(cname)	    ; \
cname:				    ; \
	STUB_ARG0(name)

#define STUB1(cname,name)             \
	.globl	cname		    ; \
	_FUNCTION(cname)	    ; \
cname:				    ; \
	STUB_ARG1(name)


#define STUB2(cname,name)             \
	.globl	cname		    ; \
	_FUNCTION(cname)	    ; \
cname:				    ; \
	STUB_ARG2(name)

	.text
	_ALIGNMENT

	/* Call functions and preserve registers */
#ifdef  UseExecstubs
	STUB0(AROS_SLIB_ENTRY(Forbid,Exec,22),AROS_CSYMNAME(_Exec_22_Forbid))
	STUB0(AROS_SLIB_ENTRY(Permit,Exec,23),AROS_CSYMNAME(_Exec_23_Permit))
	STUB0(AROS_SLIB_ENTRY(Disable,Exec,20),AROS_CSYMNAME(_Exec_20_Disable))
	STUB0(AROS_SLIB_ENTRY(Enable,Exec,21),AROS_CSYMNAME(_Exec_21_Enable))

	STUB1(AROS_SLIB_ENTRY(ObtainSemaphore,Exec,94),AROS_CSYMNAME(_Exec_94_ObtainSemaphore))
	STUB1(AROS_SLIB_ENTRY(ReleaseSemaphore,Exec,95),AROS_CSYMNAME(_Exec_95_ReleaseSemaphore))
	STUB1(AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec,113),AROS_CSYMNAME(_Exec_113_ObtainSemaphoreShared))
#endif

