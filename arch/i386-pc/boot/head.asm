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
    0x00000b00 - 0x00000b18	Global Descriptor Table - 3 entries
    0x00001000 - ....		Kernel
    ....       - 0x0009f000	Temporary stack frame for startup code.
    0x0009f000 - 0x0009ffff	BIOS private data (e.g. PS mouse block)

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
		mov	$0x0009f000,%esp

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

		sidt	idt

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
		mov	$0x0009f000,%esp	/* Put stack in safe place */
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

/* Get GDT and IDT so that you can change them */

		sgdt	gdt
		sidt	idt

/* Copy GDT to 0x00000b00 */

		movl	(gdt_base),%esi
		movl	$0x00000b00,%edi

/* Store new address */

		movl	%edi,(gdt_base)
		movl	$0x00000006,%ecx	/* 0x18/4 -> 3 GDT entries */
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

		movl	$0x00098000,%esi
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

		.globl	Set_cursor
Set_cursor:	ret


		pushl	%ebp
		movl	%esp,%ebp
		pushl	%edx
		pushl	%ecx
		pushl	%ebx
		pushl	%eax
		movl	8(%ebp),%ebx
		movw	$0x03d4,%dx
		movb	$0x0e,%al
		outb	%al,%dx
		incb	%dx
		movb	%bh,%al
		outb	%al,%dx
		decb	%dx
		movb	$0x0f,%al
		outb	%al,%dx
		incb	%dx
		movb	%bl,%al
		outb	%al,%dx
		popl	%eax
		popl	%ebx
		popl	%ecx
		popl	%edx
		popl	%ebp
		ret

gdt:
	.word	0
gdt_base:
	.long	0
idt:
	.word	0
idt_base:
	.long	0
