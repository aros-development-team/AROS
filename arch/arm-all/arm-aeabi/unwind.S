/*
 * unwind.S
 *
 *  Created on: Feb 17, 2012
 *      Author: Jason McMullan
 *  $Id$
 */

/* unwind operation stubs */
	.align 2
	.weak __aeabi_unwind_cpp_pr0
	.type __aeabi_unwind_cpp_pr0,%function
__aeabi_unwind_cpp_pr0:
	mov		r0,#0
	bx 		lr

	.align 2
	.weak __aeabi_unwind_cpp_pr1
	.type __aeabi_unwind_cpp_pr1,%function
__aeabi_unwind_cpp_pr1:
	mov		r0,#0
	bx 		lr

	.align 2
	.weak __aeabi_unwind_cpp_pr2
	.type __aeabi_unwind_cpp_pr2,%function
__aeabi_unwind_cpp_pr2:
	mov		r0,#0
	bx 		lr

/*
 * C++ unwinder hooks, referenced weakly by libgcc's ARM EHABI
 * personality routines. The AROS ELF loader rejects binaries with
 * undefined symbols - weak or not - so provide weak stubs here; a
 * real C++ runtime linked into the executable overrides them.
 */
	.align 2
	.weak __cxa_begin_cleanup
	.type __cxa_begin_cleanup,%function
__cxa_begin_cleanup:
	mov		r0,#0
	bx 		lr

	.align 2
	.weak __cxa_call_unexpected
	.type __cxa_call_unexpected,%function
__cxa_call_unexpected:
	bx 		lr
