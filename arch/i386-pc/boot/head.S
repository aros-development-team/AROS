/*
    (C) 1997-1999 AROS - The Amiga Research OS
    $Id$

    Desc: Kernel header for standalone AROS on PC
    Lang: English
*/

/*
    Memory in AROS looks like this:

    0x00000000.l		nothing
    0x00000004.l		ExecBase

    *** start of SYSTEM USE ONLY area ***

    0x00000008 - 0x00000808	int vectors
    0x00000900 - 0x00000b00	data from setup
    0x00000b00 - 0x00000fff	Global Descriptor Table - 5 entries
    0x00001000 - ....		Kernel - if small
    ....       - 0x00090fff	Temporary stack frame for startup code.
    0x00093000 - 0x0009ffff	BIOS private data (e.g. PS mouse block)

    *** end of SYSTEM USE ONLY area ***

    0x00100000 - 0x00ffffff	24BIT DMA Memory area
    0x01000000 - 0x........	Rest of memory

    History

    1.4 	Removed reload_flag. It's useless.

*/

.text

#define __ASSEMBLY__
#include <aros/config.h>
#include <asm/segments.h>

		.globl	startup_32
startup_32:
		jmp	start	/* Skip header */

		.word	41,5	/* kernel version */

		.ascii	"AMIGA Research Operating System (AROS)"
		.byte	0
		.ascii	"Copyright (C) 1995-1999"
		.byte	0
		.ascii	"AROS - The Amiga Research OS "
		.byte	0
		.ascii	"ALPHA "
		.byte	0

		.balign 16

/* Restart after soft reset here */

		.globl	start_again

start_again:	cld
		cli
		movl	$KERNEL_DS,%eax
		mov	%ax,%ds
		mov	%ax,%es
		mov	%ax,%fs
		mov	%ax,%gs
		mov	%ax,%ss
		mov	$0x00093000,%esp

		pushl	$0	/* Clear all unexcepted bits in EFLAGS */
		popf

/* Set all colors to black */

		movl	$0x3c8,%edx
		movb	$0,%al
		outb	%al,%dx
		incb	%dl
		movl	$768,%ecx
__clear:	outb	%al,%dx
		nop
		nop
		nop
		nop
		nop
		nop
		decl	%ecx
		jne	__clear

/* Clear BSS */
		xorl	%eax,%eax
		movl	$_edata,%edi
		movl	$_end,%ecx
		subl	%edi,%ecx
		cld
		rep
		stosb

		movl	$0x00000008,%eax
		movl	%eax,(idt_base)
		movw	$0x07ff,%ax
		mov	%ax,(idt)

/* Clear first 4 bytes */

		xorl	%eax,%eax
		movl	%eax,(0x00000000)

		call	make_ientry
		movl	$0x00000008,%edi
		movl	(int_entry),%eax
		movl	(4+int_entry),%ebx
		movl	$256,%ecx

/* Fill interrupt table */

copy_idts_2:	movl	%eax,(%edi)
		movl	%ebx,4(%edi)
		lea	8(%edi),%edi
		decl	%ecx
		jne	copy_idts_2

		lidt	idt

/* System timer to 50Hz */

		mov	$23864,%eax
		outb	%al,$0x40
		xchg	%ah,%al
		outb	%al,$0x40

		call	checkCPUtype

/* Call C-code kernel */

		call	main
		jmp	dummy_halt

start:		cld
		cli

/* Set segment registers up */

		movl	$KERNEL_DS,%eax 	/* Fill segment registers */
		mov	%ax,%ds 		/* All are the same - one */
		mov	%ax,%es 		/* 4GB data segment */
		mov	%ax,%fs
		mov	%ax,%gs
		mov	%ax,%ss
		mov	$0x00093000,%esp	/* Put stack in safe place */
						/* skip BIOS ext area */

		pushl	$0	/* Clear all unexcepted bits in EFLAGS */
		popfl

/* Clear BSS */
		xorl	%eax,%eax
		movl	$_edata,%edi
		movl	$_end,%ecx
		subl	%edi,%ecx
		cld
		rep
		stosb

/* Copy GDT to 0x00000b00 */

		movl	$gdt_d,%esi
		movl	$0x00000b00,%edi

/* Store new address */

		movl	%edi,(gdt_base)
		movl	$0x0000000a,%ecx	/* 0x18/4 -> 3 GDT entries */
		cld
		rep
		movsl

/* Set IDT base up */

		movl	$0x00000008,%eax
		movl	%eax,(idt_base)
		movw	$0x7ff,%ax	/* Store IDT size */
		movw	%ax,(idt)

/* Clear first 4 bytes */

		xorl	%eax,%eax
		movl	%eax,(0x00000000)
/*
    Don't touch ExecBase. We'll try (maybe) leter to detect whether OS starts
    first time or after soft reset - We'll control soft resets - Yuppi!!! :-))
*/

		call	make_ientry
		movl	$0x00000008,%edi
		movl	(int_entry),%eax
		movl	(4+int_entry),%ebx
		movl	$256,%ecx

/* Fill interrupt table */

copy_idts:	movl	%eax,(%edi)
		movl	%ebx,4(%edi)
		lea	8(%edi),%edi
		decl	%ecx
		jne	copy_idts

		lgdt	gdt
		lidt	idt

/* Copy data from setup */

		movl	$0x00090000,%esi
		movl	$0x00000900,%edi
		movl	$128,%ecx	/* 512/4 */
		cld
		rep
		movsl

/* Turn off floppy led */

		movw	$0x3f2,%dx
		xorb	%al,%al
		outb	%al,%dx

/* Set the timer to 50Hz */

		mov	$23864,%eax	/* 1193180/50 */
		outb	%al,$0x40	/* We don't need sending any comands */
		xchg	%ah,%al 	/* BIOS did it for us */
		outb	%al,$0x40	/* It was a bit dangerous ... */

		call	checkCPUtype

/* OK, head has finished its job. Go to the exec now */
/* In future there will be "jmp ..." instead of "call" */

		call	main

/* PC will reach this instruction only if main function is uncompleted */

dummy_halt:	jmp	dummy_halt

/* Dummy interrupt entry in case you use some "int ..." instructions */

empty_int:	iret

/* Fill temporary int entry */

make_ientry:	lea	(empty_int),%edx
		lea	(int_entry),%edi

		movw	%dx,(%edi)
		rorl	$16,%edx		/* Split offset */
		movw	$KERNEL_CS,2(%edi)      /* Code segment */
		movw	$0x8e00,4(%edi)         /* Type = interrupt */
		movw	%dx,6(%edi)
		ret

int_entry:	.long	0
		.long	0


/*
 Guess MemorySize - function used only by init.c
 Moved here because of some problems with asm and c merge
*/

		.globl	GetMemSize
GetMemSize:	pushl	%edi			/* Save temp used reg */
		movl    $0x00100000,%edi	/* start address */
.lmm1:		movl    (%edi),%eax		/* Save, write, test and restore... */
		movl    $0xdeadbeef,(%edi)
		cmpl    $0xdeadbeef,(%edi)
		movl    %eax,(%edi)
		jne     .lmm2			/* We found unusable memory - finish */
		addl    $16,%edi
		jmp     .lmm1
.lmm2:		movl    %edi,%eax		/* Store first nonexisting byte addr */
		popl	%edi			/* That's it! Short and fast ;-) */
		ret
		
/* Detecting CPU type - this code has bee taken directly from linux sources */


/*
 * References to members of the boot_cpu_data structure.
 */

#define CPU_PARAMS	0x000009a0
#define X86		CPU_PARAMS+0
#define X86_VENDOR	CPU_PARAMS+1
#define X86_MODEL	CPU_PARAMS+2
#define X86_MASK	CPU_PARAMS+3
#define X86_HARD_MATH	CPU_PARAMS+6
#define X86_CPUID	CPU_PARAMS+8
#define X86_CAPABILITY	CPU_PARAMS+12
#define X86_VENDOR_ID	CPU_PARAMS+16

checkCPUtype:

	movl $-1,X86_CPUID		#  -1 for no CPUID initially

/* check if it is 486 or 386. */
/*
 * XXX - this does a lot of unnecessary setup.  Alignment checks don't
 * apply at our cpl of 0 and the stack ought to be aligned already, and
 * we don't need to preserve eflags.
 */

	movl $3,X86		# at least 386
	pushfl			# push EFLAGS
	popl %eax		# get EFLAGS
	movl %eax,%ecx		# save original EFLAGS
	xorl $0x40000,%eax	# flip AC bit in EFLAGS
	pushl %eax		# copy to EFLAGS
	popfl			# set EFLAGS
	pushfl			# get new EFLAGS
	popl %eax		# put it in eax
	xorl %ecx,%eax		# change in flags
	andl $0x40000,%eax	# check if AC bit changed
	je is386

	movl $4,X86		# at least 486
	movl %ecx,%eax
	xorl $0x200000,%eax	# check ID flag
	pushl %eax
	popfl			# if we are on a straight 486DX, SX, or
	pushfl			# 487SX we can't change it
	popl %eax
	xorl %ecx,%eax
	pushl %ecx		# restore original EFLAGS
	popfl
	andl $0x200000,%eax
	je is486

	/* get vendor info */
	xorl %eax,%eax			# call CPUID with 0 -> return vendor ID
	cpuid
	movl %eax,X86_CPUID		# save CPUID level
	movl %ebx,X86_VENDOR_ID		# lo 4 chars
	movl %edx,X86_VENDOR_ID+4	# next 4 chars
	movl %ecx,X86_VENDOR_ID+8	# last 4 chars

	orl %eax,%eax			# do we have processor info as well?
	je is486

	movl $1,%eax		# Use the CPUID instruction to get CPU type
	cpuid
	movb %al,%cl		# save reg for future use
	andb $0x0f,%ah		# mask processor family
	movb %ah,X86
	andb $0xf0,%al		# mask model
	shrb $4,%al
	movb %al,X86_MODEL
	andb $0x0f,%cl		# mask mask revision
	movb %cl,X86_MASK
	movl %edx,X86_CAPABILITY

is486:
	movl %cr0,%eax		# 486 or better
	andl $0x80000011,%eax	# Save PG,PE,ET
	orl $0x50022,%eax	# set AM, WP, NE and MP
	jmp 2f

is386:	pushl %ecx		# restore original EFLAGS
	popfl
	movl %cr0,%eax		# 386
	andl $0x80000011,%eax	# Save PG,PE,ET
	orl $2,%eax		# set MP
2:	movl %eax,%cr0
	call check_x87
	xorl %eax,%eax
	lldt %ax
	cld			# gcc2 wants the direction flag cleared at all times
	ret

/*
 * We depend on ET to be correct. This checks for 287/387.
 */
check_x87:
	movb $0,X86_HARD_MATH
	clts
	fninit
	fstsw %ax
	cmpb $0,%al
	je 1f
	movl %cr0,%eax		/* no coprocessor: have to set bits */
	xorl $4,%eax		/* set EM */
	movl %eax,%cr0
	ret
	.align 0
1:	movb $1,X86_HARD_MATH
	.byte 0xDB,0xE4		/* fsetpm for 287, ignored by 387 */
	ret

	.balign 32
gdt_d:
        .word   0,0,0,0         /* dummy */
 
        .word   0xffff          /* 4GB */
        .word   0x0000          /* base address = 0 */
        .word   0x9a00          /* code read/exec */
        .word   0x00cf          /* granularity=4096, 386 descriptor */
 
        .word   0xffff          /* same as abowe except type */
        .word   0x0000
        .word   0x9200          /* data read/write */
        .word   0x00cf
 
        .word   0xffff          
        .word   0x0000          /* The same two for user segments */
        .word   0xfa00          
        .word   0x00cf          
 
        .word   0xffff
        .word   0x0000
        .word   0xf200
        .word   0x00cf                                                                                     

	.word 	-1
gdt:
	.word	40
gdt_base:
	.long	gdt_d

	.word	-1
idt:
	.word	0
idt_base:
	.long	0
