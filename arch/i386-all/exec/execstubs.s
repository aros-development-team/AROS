/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Stubs to call C functions while preserving all registers
    Lang: english
*/
	#include "machine.i"

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

#define NUM_REGS    7
#define FIRST_ARG   ((NUM_REGS+1)*4)
#define SECOND_ARG  (FIRST_ARG+4)

#define PUSH			      \
	pushl %eax		    ; \
	pushl %ebx		    ; \
	pushl %ecx		    ; \
	pushl %edx		    ; \
	pushl %esi		    ; \
	pushl %edi		    ; \
	pushl %ebp

#define POP			      \
	popl  %ebp		    ; \
	popl  %edi		    ; \
	popl  %esi		    ; \
	popl  %edx		    ; \
	popl  %ecx		    ; \
	popl  %ebx		    ; \
	popl  %eax

#define STUB_ARG0(name)               \
	PUSH			    ; \
	call  name		    ; \
	POP			    ; \
	ret

#define STUB_ARG1(name)               \
	PUSH			    ; \
	movl  FIRST_ARG(%esp),%eax  ; \
	pushl %eax		    ; \
	call  name		    ; \
	addl  $4,%esp		    ; \
	POP			    ; \
	ret

#define STUB_ARG2(name)               \
	PUSH			    ; \
	movl  SECOND_ARG(%esp),%eax ; \
	pushl %eax		    ; \
	movl  SECOND_ARG(%esp),%eax ; \
	pushl %eax		    ; \
	call  name		    ; \
	addl  $8,%esp		    ; \
	POP			    ; \
	ret

/* To save typing work */
#define STUB0(cname,name)             \
	.globl	cname		    ; \
	.type	cname,@function     ; \
cname:				    ; \
	STUB_ARG0(name)

#define STUB1(cname,name)             \
	.globl	cname		    ; \
	.type	cname,@function     ; \
cname:				    ; \
	STUB_ARG1(name)


#define STUB2(cname,name)             \
	.globl	cname		    ; \
	.type	cname,@function     ; \
cname:				    ; \
	STUB_ARG2(name)

	.text
	.balign 4

	/* Call functions and preserve registers */
	STUB0(AROS_CDEFNAME(os_disable),AROS_CSYMNAME(_os_disable))
	STUB0(AROS_CDEFNAME(os_enable),AROS_CSYMNAME(_os_enable))

	STUB1(AROS_SLIB_ENTRY(Forbid,Exec),_Forbid)
	STUB1(AROS_SLIB_ENTRY(Permit,Exec),_Permit)
	STUB1(AROS_SLIB_ENTRY(Disable,Exec),_Disable)
	STUB1(AROS_SLIB_ENTRY(Enable,Exec),_Enable)

	STUB2(AROS_SLIB_ENTRY(ObtainSemaphore,Exec),AROS_CSYMNAME(_ObtainSemaphore))
	STUB2(AROS_SLIB_ENTRY(ReleaseSemaphore,Exec),AROS_CSYMNAME(_ReleaseSemaphore))
	STUB2(AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec),AROS_CSYMNAME(_ObtainSemaphoreShared))

#if 1
	.globl	AROS_SLIB_ENTRY(Switch,Exec)
	.type	AROS_SLIB_ENTRY(Switch,Exec),@function
AROS_SLIB_ENTRY(Switch,Exec):
	/* Make room for Dispatch() address. */
	subl	$4,%esp

	/* Preserve registers */
	PUSH

	movl	SECOND_ARG(%esp),%ebx

#if 1
	/* If current state is TS_RUN and TF_EXCEPT is 0...
	    The andb will clear all bits except TF_EXCEPT when it is
	    set and then the cmpw will fail if the bit it set.
	*/
	movl	ThisTask(%ebx),%ecx
	movw	tc_Flags(%ecx),%eax
	andb	$TF_EXCEPT,%al
	cmpw	$TS_RUN*256,%ax
	jne	1f  /* %ax != TS_RUN*256 */

	/* ...move task to the ready list */
	movb	$TS_READY,tc_State(%ecx)
	leal	Enqueue(%ebx),%edx
	pushl	%ebx
	pushl	%ecx
	leal	TaskReady(%ebx),%eax
	pushl	%eax
	call	*%edx
	addl	$12,%esp

1:
#else
/* ??? This code doesn't work. Eventually the TaskReady list will be
   overwritten with NULL and crash. */
	pushl	%ebx
	call	AROS_CSYMNAME(_Switch)
	addl	$4,%esp

	movl	SECOND_ARG(%esp),%ebx
#endif

	/* Prepare dispatch */
	leal	AROS_SLIB_ENTRY(Dispatch,Exec),%ecx
	movl	%ecx,(NUM_REGS*4)(%esp)

	/* restore registers and dispatch */
	POP
	ret
#endif
