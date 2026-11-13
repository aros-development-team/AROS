/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2008 function siglongjmp()
    Lang: english
*/

	#include "aros/i386/asm.h"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(siglongjmp)
	_FUNCTION(AROS_CDEFNAME(siglongjmp))

	.set	FirstArg, 4 /* Skip return-address */
	.set	env, FirstArg
	.set	val, env+4

AROS_CDEFNAME(siglongjmp):
	/* Fetch the address of the env-structure off the stack.
	    The address is stored in %eax which is not preserved
	    because it's contents are overwritten anyway by the
	    return code */
	movl env(%esp),%eax

	/* Read return value into %ebx and make sure it's not 0 */
	movl val(%esp),%ebx
	cmpl $0,%ebx
	jne  1f

	movl $1,%ebx
1:
	/* Restore stack pointer and all registers from env */
	movl 28(%eax),%esp /* Restore original stack */

	movl 0(%eax),%ecx
	movl %ecx,retaddr(%esp) /* Restore return address */

	pushl %ebx /* Save return value on new stack */

	/* Restore all registers */
	movl 4(%eax),%ebx /* %ebx */
	movl 8(%eax),%ecx /* %ecx */
	movl 12(%eax),%edx /* %edx */
	movl 16(%eax),%esi /* %esi */
	movl 20(%eax),%edi /* %edi */
	movl 24(%eax),%ebp /* %ebp */

	popl %eax /* Fetch return value */
	ret
