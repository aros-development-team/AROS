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

                /* StackSwap -> %rdi */
                movq    %rbx, %rdi
                popq    %rbx
                
                /* Get the return address */
                popq    %rax
                
                /* Restore *NEW* stack pointer */
                xchgq   %rsp, stk_Pointer(%rdi)
                
                pushq   %rax
                call    *Enable(%rsi)
                popq    %rax
                
                jmpq    *%rax
                                