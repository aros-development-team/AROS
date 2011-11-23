/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Stubs to call C functions while preserving all registers, x86-64 version
    Lang: english
*/
	#include "aros/x86_64/asm.h"

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

/*
 * r12 doesn't have to be preserved, however i need to keep stack
 * alignment. So, i push r12.
 * x86-64 ABI always suggests 16-byte-aligned stack. 64-bit Windows
 * crashes in RaiseException() (used to implement kernel syscalls)
 * if this requirement is not met.
 */
#define PUSH			      \
	pushq %rax		    ; \
	pushq %rcx		    ; \
	pushq %rdx		    ; \
	pushq %rsi		    ; \
	pushq %rdi		    ; \
	pushq %r8		    ; \
	pushq %r9		    ; \
	pushq %r10		    ; \
	pushq %r11                  ; \
	pushq %r12

#define POP			      \
        popq %r12                   ; \
	popq %r11		    ; \
	popq %r10		    ; \
	popq %r9		    ; \
	popq %r8		    ; \
	popq %rdi		    ; \
	popq %rsi		    ; \
	popq %rdx		    ; \
	popq %rcx		    ; \
	popq %rax

#define STUB_ARG0(name)              \
	push %rbp                  ; \
	mov  %rsp,%rbp             ; \
	PUSH			   ; \
	call  name		   ; \
	POP			   ; \
	leave			   ; \
	ret

#define STUB_ARG1(name)              \
	push %rbp                  ; \
	mov  %rsp,%rbp             ; \
	PUSH			   ; \
	call  name		   ; \
	POP			   ; \
	leave			   ; \
	ret

#define STUB_ARG2(name)              \
	push %rbp                  ; \
	mov  %rsp,%rbp             ; \
	PUSH			   ; \
	call  name		   ; \
	POP			   ; \
	leave			   ; \
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
	STUB1(AROS_SLIB_ENTRY(Disable,Exec,20),AROS_CSYMNAME(_Exec_20_Disable))
	STUB1(AROS_SLIB_ENTRY(Enable,Exec,21),AROS_CSYMNAME(_Exec_21_Enable))
	STUB1(AROS_SLIB_ENTRY(Forbid,Exec,22),AROS_CSYMNAME(_Exec_22_Forbid))
	STUB1(AROS_SLIB_ENTRY(Permit,Exec,23),AROS_CSYMNAME(_Exec_23_Permit))

	STUB2(AROS_SLIB_ENTRY(ObtainSemaphore,Exec,94),AROS_CSYMNAME(_Exec_94_ObtainSemaphore))
	STUB2(AROS_SLIB_ENTRY(ReleaseSemaphore,Exec,95),AROS_CSYMNAME(_Exec_95_ReleaseSemaphore))
	STUB2(AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec,113),AROS_CSYMNAME(_Exec_113_ObtainSemaphoreShared))
#endif

