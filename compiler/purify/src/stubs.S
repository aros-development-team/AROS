
#define STUB0(name)                     \
.globl _##name			      ;  \
	.type _##name,@function       ;  \
_##name:			      ;  \
	pushl %eax		      ;  \
	pushl %ebx		      ;  \
	pushl %ecx		      ;  \
	pushl %edx		      ;  \
	pushl %edi		      ;  \
	pushl %esi		      ;  \
	call  name		      ;  \
	popl  %esi		      ;  \
	popl  %edi		      ;  \
	popl  %edx		      ;  \
	popl  %ecx		      ;  \
	popl  %ebx		      ;  \
	popl  %eax		      ;  \
	ret


#define STUB2(name)                     \
.globl _##name			      ;  \
	.type _##name,@function       ;  \
_##name:			      ;  \
	pushl %ebp		      ;  \
	movl  %esp,%ebp 	      ;  \
	pushl %eax		      ;  \
	pushl %ebx		      ;  \
	pushl %ecx		      ;  \
	pushl %edx		      ;  \
	pushl %edi		      ;  \
	pushl %esi		      ;  \
	pushl 12(%ebp)                ;  \
	pushl 8(%ebp)                 ;  \
	call  name		      ;  \
	addl  $8,%esp		      ;  \
	popl  %esi		      ;  \
	popl  %edi		      ;  \
	popl  %edx		      ;  \
	popl  %ecx		      ;  \
	popl  %ebx		      ;  \
	popl  %eax		      ;  \
	movl  %ebp,%esp 	      ;  \
	popl  %ebp		      ;  \
	ret

#define STUB4(name)                     \
.globl _##name			      ;  \
	.type _##name,@function       ;  \
_##name:			      ;  \
	pushl %ebp		      ;  \
	movl  %esp,%ebp 	      ;  \
	pushl %eax		      ;  \
	pushl %ebx		      ;  \
	pushl %ecx		      ;  \
	pushl %edx		      ;  \
	pushl %edi		      ;  \
	pushl %esi		      ;  \
	pushl 20(%ebp)                ;  \
	pushl 16(%ebp)                ;  \
	pushl 12(%ebp)                ;  \
	pushl 8(%ebp)                 ;  \
	call  name		      ;  \
	addl  $16,%esp		      ;  \
	popl  %esi		      ;  \
	popl  %edi		      ;  \
	popl  %edx		      ;  \
	popl  %ecx		      ;  \
	popl  %ebx		      ;  \
	popl  %eax		      ;  \
	movl  %ebp,%esp 	      ;  \
	popl  %ebp		      ;  \
	ret

STUB2(Purify_Push)
STUB2(Purify_Pop)
STUB2(Purify_Alloca)
STUB2(Purify_MoveSP)
STUB2(Purify_CheckRead)
STUB2(Purify_CheckWrite)
STUB2(Purify_InitStack)
STUB0(Purify_Init)
STUB0(Purify_Exit)
STUB0(Purify_LeaveFunction)
STUB4(Purify_EnterFunction)


