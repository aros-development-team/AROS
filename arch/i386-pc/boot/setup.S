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

    0x0a0   B	    X86 model: 3=386, 4=486...
    0x0a2   B       Model
    0x0a3   B       Mask
    0x0a8   L       CPUID level
    0x0ac   L	    Capability
    0x0b0   0x0c    Vendor ID

    ...

    0x1ff   B	    Pointing device indicator. 0xaa - device installed
*/

start_of_setup:

! Detect CPU type. 0=8088 1=v20 2=80186 3=80286...

		mov	ax,-1
		mov	(0xa8),ax
		mov	(0xaa),ax

		pushf
		cli
		push	dx
		push	ax
		push	cx
		mov	cl,#0x20	! 80186+ masks bits 5-7 at shifts,
		mov	al,#0x01	! then shr arg,0x20 = nop. For older
		shr	al,cl		! cpus al=0
		test	al,al
		jnz	detect_186
		mov	al,#0x40
		mul	al,al		! V30 don't clear ZF after multily
		jz	detect_v30
detect_086:	xor	dl,dl		! 8086 or 8088
		jmp	detect_e1
detect_v30:	mov	dl,#0x00	! V20 or V30
		jmp	detect_e1
detect_186:	push	sp		! 80286+ remembers sp before push
		pop	ax		! older cpus - after
		cmp	ax,sp
		je	detect_286
		mov	dl,#0x01	! 80186 or 80188
		jmp	detect_e1
detect_286:	pushf
		pushf
		pop	ax
		or	ah,#0x40	! 80286 can't change NT flag in
		push	ax		! real mode, 80386+ can do it
		popf
		pushf
		pop	ax
		popf
		test	ah,#0x40
		jnz	detect_386
		mov	dl,#0x02	! 80286
detect_e1:	jmp	detect_e2

detect_386:	mov	cx,sp		! Don't cause exception 0x11
		and	sp,#-4
		pushfd			! Test AC flag (bit 18). It can be
		pushfd			! changed in 80486+
		pop	eax
		btc	eax,#0x12
		setc	dh
		push	eax
		popfd
		pushfd
		pop	eax
		popfd
		mov	sp,cx
		shl	eax,#0x0e
		sbb	dh,#0
		jnz	detect_486
		mov	dl,#0x03	! 80386
detect_e2:	jmp	detect_end
detect_486:	pushfd
		pushfd			! Check ID flag (bit 21)
		pop	eax
		btc	eax,#0x15
		setc	dh		! It can be changed in some 486 and
		push	eax		! all 80586+ - this cpus support
		popfd			! cpuid instruction
		pushfd
		pop	eax
		popfd
		shl	eax,#0x0b
		sbb	dh,#0
		jnz	detect_pid
found_486:	mov	dl,#0x04	! 80486
		jmp	detect_end

detect_pid:	pushad
		xor	eax,eax		! Check number of cpuid commands
		.byte	0x0f,0xa2	! CPUID instruction

		mov	(0xa8),eax	! Store CPUID level
		mov	(0xb0),ebx	! Store Vendor ID first 4 chars
		mov	(0xb4),edx	! the same, middle 4 chars
		mov	(0xb8),ecx	! last 4 chars
		
		test	eax,eax		! Is this a 486?
		jz	found_486
		
		xor	eax,eax		! Get CPU type
		inc	eax
		.byte	0x0f,0xa2
		
		mov	cl,al
		and	ah,#0x0f	! Processor family
		mov	(0xa0),ah
		and	al,#0xf0	! Processor model
		shr	al,#4
		mov	(0xa2),al
		and	cl,#0x0f	! Revision
		mov	(0xa3),cl
		mov	(0xac),edx	! Capabilities
		mov	dl,(0xa0)

detect_end:	pop	cx
		pop	ax
		mov	al,dl
		cbw
		pop	dx
		popf

! Detection is done. Now we have CPU info in ax

		mov	(0x00),ax	! Store CPU info
		mov	(0xa0),al
		cmp	ax,#3
		jae	cpu_ok
		push	cs
		pop	dx
		mov	si,#bad_cpu
		call	prtstr
cpu_hlt:	jmp	cpu_hlt
cpu_ok:

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
		.byte	0

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

idt_48:
		.word	0		! idt limit = 0
		.word	0,0		! idt base = 0

gdt_48:
		.word	3*8-1		! limit=24 -> 3 entries
		.long	0x98200 + gdt

bad_cpu:
		.byte	13,10
		.ascii	"AROS needs 80386 CPU or better."
		.byte	13,10,0
		
.text
endtext:
.data
enddata:
.bss
endbss:
