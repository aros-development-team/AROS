/*
    (C) 1997-98 AROS - The Amiga Research OS
    $Id$
    
    Desc: System setup for standalone AROS on PC
    Lang: English
*/

/*****************************************************************************

    FUNCTION
	Almost the same as in linux. It gets all needed data from BIOS and
	puts them into safe place, so that 32-bit kernel know everything
	about our PC. This module is loaded by bootblock.

	This version IS BASED ON LINUX VERSION!!!
	
	List of things setup has to do:
	
	 0. Check for CPU. AROS needs 80386 or better.
	 1. Reset HDD controler.
	 2. Check RAM amount. <-- rather invalid.
	 3. Init video card.
	 4. Get hd0 data.
	 5. Get hd1 data if device exists.
	 6. Check PS/2 pointing device.
	 7. Check for APM BIOS.
	 8. Set up IDT and GDT tables.
	 9. Enable A20 control.
	10. Reset fpu.
	11. Set up IRQ.
	12. Turn on flat protected mode.
	13. Jump to kernel at 0x00001000.

    HISTORY
	1.4	A20 switching bug REMOVED!

*****************************************************************************/

#define __ASSEMBLY__
#include <aros/boot.h>
#include <asm/segments.h>

INITSEG       = DEF_INITSEG		! 0x9800 - there is boot code
SYSSEG        = DEF_SYSSEG     		! 0x0100 - there is kernel
SETUPSEG      = DEF_SETUPSEG 		! 0x9820 - there are we.
DELTA_INITSEG = SETUPSEG - INITSEG 	! 0x0020

.globl	begtext, begdata, begbss, endtext, enddata, endbss
.text
begtext:
.data
begdata:
.bss
begbss:
.text

entry	start
start:		jmp	start_of_setup

/****************************** DUMMY HEADER ********************************/

		.ascii	"AROS_hdr"
start_sys_seg:
		.word	SYSSEG

/***************************** END OF HEADER ********************************/

/*
    Data for kernel is placed at 0x98000 - the place where was bootblock
    Data structure:
    
    Addr:   Size:   Function:

    0x000...

    0x017   B	    VGA indicator

    0x018...

    0x040   W	    APM BIOS version. If 0 no APM BIOS supported
    0x042   W	    BIOS code segment
    0x044   L	    BIOS entry point
    0x048   W       BIOS 16 bit code segment
    0x04a   W	    BIOS data segment
    0x04c   W	    APM BIOS flags. Bit 1 means 32-bit APM is enabled
    0x04e   W       BIOS code segment length
    0x050   W	    BIOS data segment length

    0x052...

    0x080   0x10    hd0 drive table
    0x090   0x10    hd1 drive table

    Things that are set up later...

    0x0a0   B	    X86 model: 3=386, 4=486...
    0x0a2   B       Model
    0x0a3   B       Mask
    0x0a8   L       CPUID level
    0x0ac   L	    Capability
    0x0b0   0x0c    Vendor ID

    ...

    0x1ff   B	    Pointing device indicator. 0xaa - device installed
*/

start_of_setup:	/* I have removed cpu detection routine... it caused problems */

! Reset HDD controler

		mov	ax,#0x1500
		mov	dl,#0x81
		int	0x13

		mov	ax,#0x0000
		mov	dl,#0x80
		int	0x13

! Check RAM Amount

		mov	ah,#0x88
		int	0x15
		mov	(0x02),ax

! Init video card

		call	video

! Get hd0 data

		xor	ax,ax
		mov	ds,ax
		seg	ds
		lds	si,(4*0x41)
		mov	ax,cs
		sub	ax,#DELTA_INITSEG
		push	ax
		mov	es,ax
		mov	di,#0x0080
		mov	cx,#0x10
		push	cx
		cld
		rep
		movsb

! Get hd1 data if device exists

		xor	ax,ax
		mov	ds,ax
		seg	ds
		lds	si,(4*0x46)
		pop	cx
		pop	es
		mov	di,#0x0090
		rep
		movsb

		mov	ax,#0x1500
		mov	dl,#0x81
		int	0x13
		jc	no_disk1
		cmp	ah,#0x3
		je	is_disk1
no_disk1:
		mov	ax,cs
		sub	ax,#DELTA_INITSEG
		mov	es,ax
		mov	di,#0x0090
		mov	cx,#0x10
		xor	ax,ax
		cld
		rep
		stosb
is_disk1:

! Check PS/2 pointing device

		mov	ax,cs
		sub	ax,#DELTA_INITSEG
		mov	dx,ax
		mov	(0x1ff),#0	! default is no pointing device
		int	0x11		! int 0x11: equipment determination
		test	al,#0x04	! check if pointing device installed
		jz	no_psmouse
		mov	(0x1ff),#0xaa	! device present
no_psmouse:
		
! Check for APM BIOS

		mov	(64),#0		! 0 - no APM BIOS
		
		mov	ax,#0x5300	! APM BIOS installation check
		xor	bx,bx
		int	0x15
		jc	done_apm_bios	! error -> no APM BIOS
		
		cmp	bx,#0x504d	! check for "PM" signature
		jne	done_apm_bios	! no signature -> no APM BIOS
		
		mov	(64),ax		! APM version
		mov	(76),cx		! APM flags
		and	cx,#0x02	! Is 32-bit supported?
		je	done_apm_bios	! no... :-((
		
		mov	ax,#0x5304	! Disconnect just in case
		xor	bx,bx
		int	0x15
		
		mov	ax,#0x5303	! 32 bit connect
		xor	bx,bx
		int	0x15
		jc	no_32_apm_bios	! error
		
		mov	(66),ax		! BIOS code segment
		mov	(68),ebx	! BIOS entry point offset
		mov	(72),cx		! BIOS 16 bit code segment
		mov	(74),dx		! BIOS data segment
		mov	(78),si		! BIOS code segment length
		mov	(80),di		! BIOS data segment length
		jmp	done_apm_bios

no_32_apm_bios:
		and	(76),#0xfffd	! remove 32 bit support flag
done_apm_bios:

! Set up IDT and GDT tables

		cli
		mov	al,#0x80	! Lock NMI
		out	#0x70,al

		seg	cs
		lidt	idt_48		! load idt with 0,0
		seg	cs
		lgdt	gdt_48		! load gdt

! Enable A20 control

		call	empty_8042
		mov	al,#0xd1	! Command write
		out	#0x64,al
		call	empty_8042
		mov	al,#0xdf	! A20 on
		out	#0x60,al
		call	empty_8042

! Reset fpu

		xor	ax,ax
		out	#0xf0,al
		call	delay
		out	#0xf1,al
		call	delay

! Set up IRQ

! We need to remap IRQs. Stupid IBM puts some of them at 0x08-0x0f vectors.
! This area is used by internal hardware interrupts.
! We'll map all IRQs at 0x20-0x2f
! Besides all IBM problems 0x00-0x1f vectors seems to be reserved by Intel

		mov	al,#0x11	! Initialization sequence
		out	#0x20,al	! Send it to 8259A-1
		call	delay
		out	#0xa0,al	! Send it to 8259A-2
		call	delay
		mov	al,#0x20	! Start of hw IRQs 1 (0x20-0x27)
		out	#0x21,al
		call	delay
		mov	al,#0x28	! Start of hw IRQs 2 (0x28-0x2f)
		out	#0xa1,al
		call	delay
		mov	al,#0x04	! 8259A-1 is master
		out	#0x21,al
		call	delay
		mov	al,#0x02	! 8259A-2 is slave
		out	#0xa1,al
		call	delay
		mov	al,#0x01	! 8086 mode for both
		out	#0x21,al
		call	delay
		out	#0xa1,al
		call	delay
		mov	al,#0xff	! Mask off all int's now. AROS will
		out	#0xa1,al	! enable them later
		call	delay
		mov	al,#0xfb	! Enable IRQ2 - cascade to slave
		out	#0x21,al
		call	delay

! Now, when we have remaped all IRQs, we can't use BIOS anymore. It's no problem
! because it's 16 bit and useless at all. Using BIOS in AROS would kill
! multitasking, but hopefully we don't need this pice of ROM.
! Now it's time to turn on the flat protected mode.
! We'll do it just like setup.S from linux does.

		mov	eax,cr0		! Protected mode (PE) bit
		or	eax,#1
		mov	cr0,eax		! We are in protected mode

! There should be jmp instruction to empty instruction queue

! There is one more thing to do. We need to jump into kernel. Then we'll be
! in 32bit protected mode (flat). We use only 3 entries in Global Descriptor
! Table, so from this moment we can forgot about segment registers. The only
! thing we need to do with them is setting up :-))

! Jump!

		.byte	0xea
		.word	0x1000
		.word	KERNEL_CS

/*
    This command checks that the keyboard command queue is empty
    (after emptying the output buffers)
    
    No timeout is used - if this hangs there is something wrong with
    the machine, and we probably couldn't proceed anyway.
*/
empty_8042:
		call	delay
		in	al,#0x64	! 8042 status port
		test	al,#1		! output buffer?
		jz	no_output
		call	delay
		in	al,#0x60	! read it
		jmp	empty_8042
no_output:
		test	al,#2		! is input buffer full?
		jnz	empty_8042	! yes - loop
		ret

! Delay after IO
delay:
		.word	0x00eb		! jmp $+2
		ret

! Print asciiz string pointed by ds:si

prtstr:		lodsb
		and	al,al
		jz	prtend
		call	prtchr
		jmp	prtstr
prtend:		ret

! Print one char in al

prtchr:		push	ax
		push	cx
		xor	bh,bh
		mov	cx,#0x01
		mov	ah,#0x0e
		int	0x10
		pop	cx
		pop	ax
		ret

#include "video.S"
		
! Even aligment

		.align	16
!
! Descriptor tables
!

gdt:
		.word	0,0,0,0		! dummy

		.word	0xffff		! 4GB
		.word	0x0000		! base address = 0
		.word	0x9a00		! code read/exec
		.word	0x00cf		! granularity=4096, 386 descriptor

		.word	0xffff		! same as abowe except type
		.word	0x0000
		.word	0x9200		! data read/write
		.word	0x00cf

		.word	0xffff		!
		.word	0x0000		! The same two for user segments
		.word	0xfa00		!
		.word	0x00cf		!

		.word	0xffff
		.word	0x0000
		.word	0xf200
		.word	0x00cf

idt_48:
		.word	0		! idt limit = 0
		.word	0,0		! idt base = 0

gdt_48:
		.word	3*8-1		! limit=24 -> 3 entries
		.long	0x98200 + gdt

.text
endtext:
.data
enddata:
.bss
endbss:
