/*
    (C) 1997-98 AROS - The Amiga Research OS
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
    0x00000b00 - 0x00000b18	Global Descriptor Table
    0x00001000 - ....		Kernel
    ....       - 0x0009effc	Supervisor stack
    0x0009f000 - 0x0009ffff	BIOS private data (e.g. PS mouse block)

    *** end of SYSTEM USE ONLY area ***

    0x00100000 - 0x00ffffff	24BIT DMA Memory area
    0x01000000 - 0x........	Rest of memory

*/

.text

#define __ASSEMBLY__
#include <aros/config.h>
#include <asm/segments.h>

		.globl	startup_32
startup_32:
/* Set segment registers up */

		jmp	start	/* Skip header */

		.word	41,0	/* kernel version */

		.ascii	"AMIGA Research Operating System (AROS)"
		.byte	0
		.ascii	"Copyright (C) 1995-1998"
		.byte	0
		.ascii	"AROS - The Amiga Research OS "
		.byte	0
		.ascii	"ALPHA "
		.byte	0

		.balign	16

start:		cld
		cli
		movl	$(KERNEL_DS),%eax	/* Fill segment registers */
		mov	%ax,%ds			/* All are the same - one */
		mov	%ax,%es			/* 4GB data segment */
		mov	%ax,%fs
		mov	%ax,%gs
		mov	%ax,%ss
		mov	$0x0009effc,%esp	/* Put stack in safe place */
						/* skip BIOS ext area */

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

/* Clear first 4 bytes */

		xorl	%eax,%eax
		movl	%eax,(0x00000000)
/*
    Don't touch ExecBase. We'll try (maybe) leter to detect whether OS starts
    first time or after soft reset - We'll control soft resets - Yuppi!!!
*/

/*		movl	%eax,(0x00000004)		*/

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

		cmpl	$0,(reload_flag)
		jne	dont_copy
		movl	$0x00098000,%esi
		movl	$0x00000900,%edi
		movl	$128,%ecx	/* 512/4 */
		cld
		rep
		movsl
dont_copy:	incl	(reload_flag)

/* Turn off floppy led */

		movw	$0x3f2,%dx
		xorb	%al,%al
		outb	%al,%dx

/* Set the timer to 50Hz */

		mov	$23864,%eax	/* 1193180/50 */
		outb	%al,$0x40	/* We don't need sending any comands */
		xchg	%ah,%al		/* BIOS did it for us */
		outb	%al,$0x40

/*
    Uncomment this to check whether it works or not. After reset you should see
    %esp contents in left top corner of screen. It sould be 0009fffc or similar
*/

/*		movl	%esp,%edx
		movl	$0x000b8000,%edi
		movl	$8,%ecx
qqq:		roll	$4,%edx
		movb	$15,%al
		andb	%dl,%al
		addb	$0x90,%al
		daa
		adcb	$0x40,%al
		daa
		stosb
		incl	%edi
		decl	%ecx
		jne	qqq
*/
		
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
		rorl	$16,%edx
		movw	$(KERNEL_CS),2(%edi)
		movw	$0x8e00,4(%edi)
		movw	%dx,6(%edi)
		ret

int_entry:	.long	0
		.long	0

reload_flag:		/* We'll increment reload_flag every time OS restarts.
			We'll know whether to copy data from setup or not */
	.long	0

gdt:
	.word	0
gdt_base:
	.long	0
idt:
	.word	0
idt_base:
	.long	0