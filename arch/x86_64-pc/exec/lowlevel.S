#include <asm/segments.h>

#define SAVE_REGS        \
        pushq   %rax;    \
        pushq   %rdi;    \
        pushq   %rsi;    \
        pushq   %rdx;    \
        pushq   %rcx;    \
        pushq   %r8;     \
        pushq   %r9;     \
        pushq   %r10;    \
        pushq   %r11;    \
        mov     %ds,%eax; \
        pushq   %rax;    \
        pushq   %rdx;    \
        movl    $KERNEL_DS,%edx; \
        mov     %dx,%ds; \
        mov     %dx,%es; \
        popq    %rdx

#define RESTORE_REGS    \
        popq    %rax;   \
        mov     %ax,%ds;\
        mov     %ax,%es;\
        popq    %r11;   \
        popq    %r10;   \
        popq    %r9;    \
        popq    %r8;    \
        popq    %rcx;   \
        popq    %rdx;   \
        popq    %rsi;   \
        popq    %rdi;   \
        popq    %rax
        
                .balign 32,0x90
                .globl Exec_Dispatch
                .type Exec_Dispatch,@function
Exec_Dispatch:
                iretq
                .size Exec_Dispatch,.-Exec_Dispatch 

                .balign 32,0x90
                .globl core_EnterInterrupt
                .type core_EnterInterrupt,@function
core_EnterInterrupt:
                SAVE_REGS
                movq %rsp,%rdi
                
                call core_IRQHandle
                
                RESTORE_REGS
                addq $16,%rsp
                iretq
                .size core_EnterInterrupt, .-core_EnterInterrupt
                
                .balign 32,0x90
                .globl  Exec_SysCall
                .type   Exec_SysCall,@function
Exec_SysCall:   negq    %rax
                       pushq   %rax
                SAVE_REGS
#warning TODO::
                .size Exec_SysCall,.-Exec_SysCall
