/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Stubs to call C functions while preserving all registers
    Lang: english
*/
	#include "machine.i"

	.text
	.balign 16
	.globl	AROS_SLIB_ENTRY(Forbid,Exec)
	.type	AROS_SLIB_ENTRY(Forbid,Exec),@function

	.set FirstArg, 8*4	 /* 7 Registers + ret addr */

AROS_SLIB_ENTRY(Forbid,Exec):
	/* Save all registers */
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %esi
	pushl %edi
	pushl %ebp

	/* Copy SysBase */
	movl FirstArg(%esp),%eax
	pushl %eax

	/* Call function */
	call AROS_CSYMNAME(_Forbid)

	/* Clean stack */
	addl $4,%esp

	/* Restore all registers */
	popl %ebp
	popl %edi
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	ret

	.globl	AROS_SLIB_ENTRY(Permit,Exec)
	.type	AROS_SLIB_ENTRY(Permit,Exec),@function

AROS_SLIB_ENTRY(Permit,Exec):
	/* Save all registers */
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %esi
	pushl %edi
	pushl %ebp

	/* Copy SysBase */
	movl FirstArg(%esp),%eax
	pushl %eax

	/* Call function */
	call AROS_CSYMNAME(_Permit)

	/* Clean stack */
	addl $4,%esp

	/* Restore all registers */
	popl %ebp
	popl %edi
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	ret

	.globl	AROS_SLIB_ENTRY(Disable,Exec)
	.type	AROS_SLIB_ENTRY(Disable,Exec),@function

AROS_SLIB_ENTRY(Disable,Exec):
	/* Save all registers */
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %esi
	pushl %edi
	pushl %ebp

	/* Copy SysBase */
	movl FirstArg(%esp),%eax
	pushl %eax

	/* Call function */
	call AROS_CSYMNAME(_Disable)

	/* Clean stack */
	addl $4,%esp

	/* Restore all registers */
	popl %ebp
	popl %edi
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	ret

	.globl	AROS_CDEFNAME(os_disable)
	.type	AROS_CDEFNAME(os_disable),@function

AROS_CDEFNAME(os_disable):
	/* Save all registers */
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %esi
	pushl %edi
	pushl %ebp

	/* Call function */
	call AROS_CSYMNAME(_os_disable)

	/* Restore all registers */
	popl %ebp
	popl %edi
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	ret

	.globl	AROS_SLIB_ENTRY(Enable,Exec)
	.type	AROS_SLIB_ENTRY(Enable,Exec),@function

AROS_SLIB_ENTRY(Enable,Exec):
	/* Save all registers */
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %esi
	pushl %edi
	pushl %ebp

	/* Copy SysBase */
	movl FirstArg(%esp),%eax
	pushl %eax

	/* Call function */
	call AROS_CSYMNAME(_Enable)

	/* Clean stack */
	addl $4,%esp

	/* Restore all registers */
	popl %ebp
	popl %edi
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	ret

	.globl	AROS_CDEFNAME(os_enable)
	.type	AROS_CDEFNAME(os_enable),@function

AROS_CDEFNAME(os_enable):
	/* Save all registers */
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %esi
	pushl %edi
	pushl %ebp

	/* Call function */
	call AROS_CSYMNAME(_os_enable)

	/* Restore all registers */
	popl %ebp
	popl %edi
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	ret


