#define __ASSEMBLY__

#include <asm/linkage.h>
#include <asm/segments.h>

EBX		= 0x00
ECX		= 0x04
EDX		= 0x08
ESI		= 0x0C
EDI		= 0x10
EBP		= 0x14
EAX		= 0x18
DS		= 0x1C
ES		= 0x20
ORIG_EAX	= 0x24
EIP		= 0x28
CS		= 0x2C
EFLAGS		= 0x30
OLDESP		= 0x34
OLDSS		= 0x38

#define NR_syscalls 5

#define SAVE_ALL \
	cld; \
	pushl %es; \
	pushl %ds; \
	pushl %eax; \
	pushl %ebp; \
	pushl %edi; \
	pushl %esi; \
	pushl %edx; \
	pushl %ecx; \
	pushl %ebx; \
	movl $(KERNEL_DS),%edx; \
	movl %dx,%ds; \
	movl %dx,%es;

#define RESTORE_ALL	\
	popl %ebx;	\
	popl %ecx;	\
	popl %edx;	\
	popl %esi;	\
	popl %edi;	\
	popl %ebp;	\
	popl %eax;	\
	popl %ds;	\
	popl %es;	\
	addl $4,%esp;	\
	iret;

ENTRY(system_call)
	pushl	%eax			# save orig_eax
	SAVE_ALL
	cmpl 	$(NR_syscalls),%eax
	jae 	restore_all
	call 	*SYMBOL_NAME(sys_call_table)(,%eax,4)
	movl 	%eax,EAX(%esp)	# save the return value
	ALIGN
	.globl	restore_all
restore_all:
	RESTORE_ALL

	ALIGN

sys_none:
	ret

sys_Reboot:
	jmp		start_again
//	jmp	sys_Reboot

.data
ENTRY(sys_call_table)
	.long SYMBOL_NAME(sys_Cause)
	.long SYMBOL_NAME(sys_Reboot)
	.long SYMBOL_NAME(sys_none)
	.long SYMBOL_NAME(sys_none)
	.long SYMBOL_NAME(sys_none)
	.long SYMBOL_NAME(sys_none)
