/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: execstubs.s 21502 2004-04-09 00:29:38Z falemagn $

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


#define STUB_ARG0(name)               \
	push %rbp                  ; \
	mov  %rsp,%rbp             ; \
	call  name		    ; \
	leave
	ret

#define STUB_ARG1(name)               \
	push %rbp                  ; \
	mov  %rsp,%rbp             ; \
	call  name		    ; \
	leave			    ; \
	ret

#define STUB_ARG2(name)               \
	push %rbp                  ; \
	mov  %rsp,%rbp             ; \
	call  name		    ; \
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
	STUB1(AROS_SLIB_ENTRY(Forbid,Exec),AROS_CSYMNAME(_Exec_Forbid))
	STUB1(AROS_SLIB_ENTRY(Permit,Exec),AROS_CSYMNAME(_Exec_Permit))
	STUB1(AROS_SLIB_ENTRY(Disable,Exec),AROS_CSYMNAME(_Exec_Disable))
	STUB1(AROS_SLIB_ENTRY(Enable,Exec),AROS_CSYMNAME(_Exec_Enable))

	STUB2(AROS_SLIB_ENTRY(ObtainSemaphore,Exec),AROS_CSYMNAME(_Exec_ObtainSemaphore))
	STUB2(AROS_SLIB_ENTRY(ReleaseSemaphore,Exec),AROS_CSYMNAME(_Exec_ReleaseSemaphore))
	STUB2(AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec),AROS_CSYMNAME(_Exec_ObtainSemaphoreShared))
#endif

