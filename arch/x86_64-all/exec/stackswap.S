#include <aros/x86_64/asm.h>
#include <aros/config.h>

                .text
                .align  16
                .globl  AROS_SLIB_ENTRY(StackSwap, Exec)
                .type   AROS_SLIB_ENTRY(StackSwap, Exec), @function
                
AROS_SLIB_ENTRY(StackSwap, Exec):
                /* struct StackSwap is %rdi */
                /* SysBase is %rsi */
                
                pushq   %rbx
                
                /* StackSwap -> %rbx */
                movq    %rdi, %rbx
                
                /* Disable interrupts */
                pushq   %rsi
                movq	%rsi, %rdi	/* Put SysBase into rdi (first argument) */
                call    *Disable(%rsi)
                popq    %rsi

                movq    ThisTask(%rsi), %rax

                /* Exchange tc_SPLower */
                movq    tc_SPLower(%rax), %rcx
                xchgq   %rcx, stk_Lower(%rbx)
                movq    %rcx, tc_SPLower(%rax)

                /* Exchange tc_SPUpper */
                movq    tc_SPUpper(%rax), %rcx
                xchgq   %rcx, stk_Upper(%rbx)
                movq    %rcx, tc_SPUpper(%rax)

		movb	tc_Flags(%rax), %cl
		test	$TF_STACKCHK, %cl
		je	noStackSnoop

		/* Fill [stk_Lower .. stk_Pointer - 16] with 0xE1 */

		/* Destination register = %rdi = NEW SPLower.
		   Which was already swapped above, so to be found
		   in task->tc_SPLower */
		movq	tc_SPLower(%rax), %rdi

		/* %rcx = count register = NEW SP register - NEW SP_Lower - 16 */
		movq	stk_Pointer(%rbx), %rcx
		subq	$16, %rcx
		subq	%rdi, %rcx

		/* byte value to store */
		movb	$0xE1, %al

		/* direction to store: forward */
		cld

		/* do the store operation: put %rcx times %al into memory starting at %rdi. */
		rep
		stosb

noStackSnoop:
                /* StackSwap -> %rdi */
                movq    %rbx, %rdi
                popq    %rbx
                
                /* Get the return address */
                popq    %rax
                
                /* Restore *NEW* stack pointer */
                xchgq   %rsp, stk_Pointer(%rdi)
                
                pushq	%rdi
                pushq   %rax
                movq	%rsi, %rdi
                call    *Enable(%rsi)
                popq    %rax
                popq	%rdi
                
                jmpq    *%rax
                                