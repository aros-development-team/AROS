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

! Signature words to ensure LILO loaded us right
#define SIG1	0xAA55
#define SIG2	0x5A5A

INITSEG       = DEF_INITSEG		! 0x9000 - there is boot code
SYSSEG        = DEF_SYSSEG     		! 0x1000 - there is kernel
SETUPSEG      = DEF_SETUPSEG 		! 0x9020 - there are we.
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

! ------------------------ start of header --------------------------------
!
! SETUP-header, must start at CS:2 (old 0x9020:2)
!
		.ascii	"HdrS"		! Signature for SETUP-header
		.word	0x0201		! Version number of header format
					! (must be >= 0x0105
					! else old loadlin-1.5 will fail)
realmode_swtch:	.word	0,0		! default_switch,SETUPSEG
start_sys_seg:	.word	SYSSEG
		.word	kernel_version	! pointing to kernel version string
  ! note: above part of header is compatible with loadlin-1.5 (header v1.5),
  !        must not change it

type_of_loader:	.byte	0		! = 0, old one (LILO, Loadlin,
					!      Bootlin, SYSLX, bootsect...)
					! else it is set by the loader:
					! 0xTV: T=0 for LILO
					!	T=1 for Loadlin
					!	T=2 for bootsect-loader
					!	T=3 for SYSLX
					!	T=4 for ETHERBOOT
					!       V = version
loadflags:			! flags, unused bits must be zero (RFU)
LOADED_HIGH	= 1		! bit within loadflags,
				! if set, then the kernel is loaded high
CAN_USE_HEAP	= 0x80		! if set, the loader also has set heap_end_ptr
				! to tell how much space behind setup.S
				| can be used for heap purposes.
				! Only the loader knows what is free!
#ifndef __BIG_KERNEL__
		.byte	0x00
#else
		.byte	LOADED_HIGH
#endif

setup_move_size: .word  0x8000	! size to move, when we (setup) are not
				! loaded at 0x90000. We will move ourselves
				! to 0x90000 then just before jumping into
				! the kernel. However, only the loader
				! know how much of data behind us also needs
				! to be loaded.
code32_start:				! here loaders can put a different
					! start address for 32-bit code.
#ifndef __BIG_KERNEL__
		.long	0x1000		!   0x1000 = default for zImage
#else
		.long	0x100000	! 0x100000 = default for big kernel
#endif
ramdisk_image:	.long	0	! address of loaded ramdisk image
				! Here the loader (or kernel generator) puts
				! the 32-bit address were it loaded the image.
				! This only will be interpreted by the kernel.
ramdisk_size:	.long	0	! its size in bytes
bootsect_kludge:
		.word   bootsect_helper,SETUPSEG
heap_end_ptr:	.word	modelist+1024	! space from here (exclusive) down to
				! end of setup code can be used by setup
				! for local heap purposes.
! ------------------------ end of header ----------------------------------

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

! set DS=CS, we know that SETUPSEG == CS at this point
		mov	ax,cs		! aka #SETUPSEG
		mov	ds,ax

! Check signature at end of setup
		cmp	setup_sig1,#SIG1
		jne	bad_sig
		cmp	setup_sig2,#SIG2
		jne	bad_sig
		jmp	good_sig1

! Routine to print ASCIIz string at DS:SI

prtstr:		lodsb
		and	al,al
		jz	fin
		call	prtchr
		jmp	prtstr
fin:		ret

! Space printing

prtsp2:		call	prtspc		! Print double space
prtspc:		mov	al,#0x20	! Print single space (fall-thru!)

! Part of above routine, this one just prints ASCII al

prtchr:		push	ax
		push	cx
	        xor	bh,bh
		mov	cx,#0x01
		mov	ah,#0x0e
		int	0x10
		pop	cx
		pop	ax
		ret

beep:		mov	al,#0x07
		jmp	prtchr
	
no_sig_mess:	.ascii	"No setup signature found ..."
		db	0x00

good_sig1:
		jmp	good_sig

! We now have to find the rest of the setup code/data
bad_sig:
		mov	ax,cs		! aka #SETUPSEG
		sub	ax,#DELTA_INITSEG ! aka #INITSEG
		mov	ds,ax
	    	xor	bh,bh
		mov	bl,[497]	! get setup sects from boot sector
		sub	bx,#4		! LILO loads 4 sectors of setup
		shl	bx,#8		! convert to words
		mov	cx,bx
		shr	bx,#3		! convert to segment
		add	bx,#SYSSEG
		seg cs
		mov	start_sys_seg,bx

! Move rest of setup code/data to here
		mov	di,#2048	! four sectors loaded by LILO
		sub	si,si
		mov	ax,cs		! aka #SETUPSEG
		mov	es,ax
		mov	ax,#SYSSEG
		mov	ds,ax
		rep
		movsw

		mov	ax,cs		! aka #SETUPSEG
		mov	ds,ax
		cmp	setup_sig1,#SIG1
		jne	no_sig
		cmp	setup_sig2,#SIG2
		jne	no_sig
    		jmp	good_sig

no_sig:
		lea	si,no_sig_mess
		call	prtstr
no_sig_loop:
		jmp	no_sig_loop

good_sig:
		mov	ax,cs		! aka #SETUPSEG
		sub	ax,#DELTA_INITSEG ! aka #INITSEG
		mov	ds,ax

! clear setup area!!!!! For test only!

		mov	es,ax
		mov	cx,#256
		mov	di,#0
		mov	ax,#0
		cld
		rep
		stosw

! check if an old loader tries to load a big-kernel
		seg cs
		test	byte ptr loadflags,#LOADED_HIGH ! Have we a big kernel?
		jz	loader_ok	! NO, no danger even for old loaders
				! YES, we have a big-kernel
		seg cs
		cmp	byte ptr type_of_loader,#0 ! Have we one of the new loaders?
		jnz	loader_ok	! YES, OK
				! NO, we have an old loader, must give up
		push    cs
		pop	ds
		lea	si,loader_panic_mess
		call	prtstr
		jmp	no_sig_loop
loader_panic_mess: 
		.ascii	"Wrong loader:  giving up."
		db	0

loader_ok:
! Get memory size (extended mem, kB)

#ifndef STANDARD_MEMORY_BIOS_CALL
		push	ebx

	        xor     ebx,ebx		! preload new memory slot with 0k
	        mov	[0x1e0], ebx

	        mov     ax,#0xe801
		int     0x15
		jc      oldstylemem

! Memory size is in 1 k chunksizes, to avoid confusing loadlin.
! We store the 0xe801 memory size in a completely different place,
! because it will most likely be longer than 16 bits.
! (use 1e0 because that's what Larry Augustine uses in his
! alternative new memory detection scheme, and it's sensible
! to write everything into the same place.)

		and     ebx, #0xffff    ! clear sign extend
		shl     ebx, 6          ! and go from 64k to 1k chunks
		mov     [0x1e0],ebx     ! store extended memory size

		and     eax, #0xffff    ! clear sign extend
	 	add     [0x1e0],eax     ! and add lower memory into total size.
  
		! and fall into the old memory detection code to populate the
		! compatibility slot.

oldstylemem:
		pop	ebx
#else
		mov     dword ptr [0x1e0], #0
#endif
		mov	ah,#0x88
		int	0x15
		mov	[2],ax

! Init video card

		call	video

! Get hd0 data

		xor	ax,ax
		mov	ds,ax
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

! check for Micro Channel (MCA) bus
		mov ax,cs		! aka #SETUPSEG
		sub ax,#DELTA_INITSEG	! aka #INITSEG
		mov ds,ax
		mov ds,ax
		xor ax,ax
        	mov [0xa0], ax		! set table length to 0
		mov ah, #0xc0
		stc
		int 0x15		! puts feature table at es:bx
		jc no_mca
		push ds
		mov ax,es
		mov ds,ax
		mov ax,cs		! aka #SETUPSEG
		sub ax, #DELTA_INITSEG	! aka #INITSEG
		mov es,ax
		mov si,bx
		mov di,#0xa0
		mov cx,(si)
		add cx,#2		! table length is a short
		cmp cx,#0x10
		jc sysdesc_ok
		mov cx,#0x10		! we keep only first 16 bytes
sysdesc_ok:
		rep
		movsb
		pop ds

no_mca:

! Check PS/2 pointing device

		mov	ax,cs
		sub	ax,#DELTA_INITSEG
		mov	ds,ax
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
!
! Redo the installation check as the 32 bit connect
! modifies the flags returned on some BIOSs
!
		mov	ax,#0x05300	! APM BIOS installation check
		xor	bx,bx
		int	0x15
		jc	apm_disconnect	! error -> should not happen, tidy up

		cmp	bx,#0x0504d	! check for "PM" signature
		jne	apm_disconnect	! no signature -> should not happen, tidy up

		mov	[64],ax		! record the APM BIOS version
		mov	[76],cx		!	and flags

		jmp	done_apm_bios

apm_disconnect:
		mov	ax,#0x05304	! Disconnect
		xor	bx,bx
		int	0x15		! ignore return code
		jmp	done_apm_bios
no_32_apm_bios:
		and	(76),#0xfffd	! remove 32 bit support flag
done_apm_bios:

! Now we want to move to protected mode ...

        	seg cs
		cmp	realmode_swtch,#0
		jz	rmodeswtch_normal
		seg cs
		callf	far * realmode_swtch
		jmp	rmodeswtch_end
rmodeswtch_normal:
	        push	cs
		call	default_switch
rmodeswtch_end:

! we get the code32 start address and modify the below 'jmpi'
! (loader may have changed it)
		seg cs
		mov	eax,code32_start
		seg cs
		mov	code32,eax

! Now we move the system to its rightful place
! ...but we check, if we have a big-kernel.
! in this case we *must* not move it ...
		seg cs
		test	byte ptr loadflags,#LOADED_HIGH
		jz	do_move0	! we have a normal low loaded zImage
					! we have a high loaded big kernel
		jmp	end_move	! ... and we skip moving

do_move0:
		mov	ax,#0x100	! start of destination segment
		mov	bp,cs		! aka #SETUPSEG
		sub	bp,#DELTA_INITSEG ! aka #INITSEG
		seg cs
		mov	bx,start_sys_seg	! start of source segment
		cld			! 'direction'=0, movs moves forward
do_move:
		mov	es,ax		! destination segment
		inc	ah		! instead of add ax,#0x100
		mov	ds,bx		! source segment
		add	bx,#0x100
		sub	di,di
		sub	si,si
		mov 	cx,#0x800
		rep
		movsw
		cmp	bx,bp	! we assume start_sys_seg > 0x200,
				! so we will perhaps read one page more then
				! needed, but never overwrite INITSEG because
				! destination is minimum one page below source
		jb	do_move

! then we load the segment descriptors

end_move:
		mov	ax,cs ! aka #SETUPSEG
		mov	ds,ax

! If we have our code not at 0x90000, we need to move it there now.
! We also then need to move the parameters behind it (command line)
! Because we would overwrite the code on the current IP, we move
! it in two steps, jumping high after the first one.
		mov	ax,cs
		cmp	ax,#SETUPSEG
		je	end_move_self
		cli	! make sure we really have interrupts disabled !
			! because after this the stack should not be used
		sub	ax,#DELTA_INITSEG ! aka #INITSEG
		mov	dx,ss
		cmp	dx,ax
		jb	move_self_1
		add	dx,#INITSEG
		sub	dx,ax		! this will be SS after the move
move_self_1:
		mov	ds,ax
		mov	ax,#INITSEG	! real INITSEG
		mov	es,ax
		seg cs
		mov	cx,setup_move_size
		std		! we have to move up, so we use direction down
				! because the areas may overlap
		mov	di,cx
		dec	di
		mov	si,di
		sub	cx,#move_self_here+0x200
		rep
		movsb
		jmpi	move_self_here,SETUPSEG ! jump to our final place
move_self_here:
		mov	cx,#move_self_here+0x200
		rep
		movsb
		mov	ax,#SETUPSEG
		mov	ds,ax
		mov	ss,dx
				! now we are at the right place
end_move_self:



! Set up IDT and GDT tables

		lidt	idt_48		! load idt with 0,0
		lgdt	gdt_48		! load gdt

! Enable A20 control

		call	empty_8042
		mov	al,#0xd1	! Command write
		out	#0x64,al
		call	empty_8042
		mov	al,#0xdf	! A20 on
		out	#0x60,al
		call	empty_8042

! wait until a20 really *is* enabled; it can take a fair amount of
! time on certain systems; Toshiba Tecras are known to have this
! problem.  The memory location used here is the int 0x1f vector,
! which should be safe to use; any *unused* memory location < 0xfff0
! should work here.  

#define	TEST_ADDR 0x7c

		push	ds
		xor	ax,ax			! segment 0x0000
		mov	ds,ax
		dec	ax			! segment 0xffff (HMA)
		mov	gs,ax
		mov	bx,[TEST_ADDR]		! we want to restore the value later
a20_wait:
		inc	ax
		mov	[TEST_ADDR],ax
		seg	gs
		cmp	ax,[TEST_ADDR+0x10]
		je	a20_wait		! loop until no longer aliased
		mov	[TEST_ADDR],bx		! restore original value
		pop	ds
	
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

		mov	ax,#1		! protected mode (PE) bit
		lmsw	ax		! This is it!
		jmp	flush_instr
flush_instr:
		xor	bx,bx		! Flag to indicate a boot

! There is one more thing to do. We need to jump into kernel. Then we'll be
! in 32bit protected mode (flat). We use only 3 entries in Global Descriptor
! Table, so from this moment we can forgot about segment registers. The only
! thing we need to do with them is setting up :-))

! Jump!

		db	0x66,0xea	! prefix + jmpi-opcode
code32:		dd	0x1000		! will be set to 0x100000 for big kernels
		dw	KERNEL_CS

kernel_version:	.ascii	"AMIGA Research Operating System (AROS)"
		db	0

! This is the default real mode switch routine.
! to be called just before protected mode transition

default_switch:
		cli			! no interrupts allowed !
		mov	al,#0x80	! disable NMI for the bootup sequence
		out	#0x70,al
		retf

! This routine only gets called, if we get loaded by the simple
! bootsect loader _and_ have a bzImage to load.
! Because there is no place left in the 512 bytes of the boot sector,
! we must emigrate to code space here.
!
bootsect_helper:
		seg cs
		cmp	word ptr bootsect_es,#0
		jnz	bootsect_second
		seg cs
		mov	byte ptr type_of_loader,#0x20
		mov	ax,es
		shr	ax,#4
		seg	cs
		mov	byte ptr bootsect_src_base+2,ah
		mov	ax,es
		seg cs
		mov	bootsect_es,ax
		sub	ax,#SYSSEG
		retf			! nothing else to do for now
bootsect_second:
		push	cx
		push	si
		push	bx
		test	bx,bx	! 64K full ?
		jne	bootsect_ex
		mov	cx,#0x8000	! full 64K move, INT15 moves words
		push	cs
		pop	es
		mov	si,#bootsect_gdt
		mov	ax,#0x8700
		int	0x15
		jc	bootsect_panic	! this, if INT15 fails
		seg cs
		mov	es,bootsect_es	! we reset es to always point to 0x10000
		seg cs
		inc	byte ptr bootsect_dst_base+2
bootsect_ex:
		seg cs
		mov	ah, byte ptr bootsect_dst_base+2
		shl	ah,4	! we now have the number of moved frames in ax
		xor	al,al
		pop	bx
		pop	si
		pop	cx
		retf

bootsect_gdt:
		.word	0,0,0,0
		.word	0,0,0,0
bootsect_src:
		.word	0xffff
bootsect_src_base:
		.byte	0,0,1			! base = 0x010000
		.byte	0x93			! typbyte
		.word	0			! limit16,base24 =0
bootsect_dst:
		.word	0xffff
bootsect_dst_base:
		.byte	0,0,0x10		! base = 0x100000
		.byte	0x93			! typbyte
		.word	0			! limit16,base24 =0
		.word	0,0,0,0			! BIOS CS
		.word	0,0,0,0			! BIOS DS
bootsect_es:
		.word	0

bootsect_panic:
		push	cs
		pop	ds
		cld
		lea	si,bootsect_panic_mess
		call	prtstr
bootsect_panic_loop:
		jmp	bootsect_panic_loop
bootsect_panic_mess:
		.ascii	"INT15 refuses to access high memory.  Giving up."
		db	0

/*
    This command checks that the keyboard command queue is empty
    (after emptying the output buffers)
    
    No timeout is used - if this hangs there is something wrong with
    the machine, and we probably couldn't proceed anyway.
*/

empty_8042:
		push    ecx
		mov     ecx,#0xFFFFFF

empty_8042_loop:
		dec     ecx
		jz      empty_8042_end_loop

		call	delay
		in	al,#0x64	! 8042 status port
		test	al,#1		! output buffer?
		jz	no_output
		call	delay
		in	al,#0x60	! read it
		jmp	empty_8042_loop
no_output:
		test	al,#2		! is input buffer full?
		jnz	empty_8042_loop	! yes - loop
empty_8042_end_loop:
	        pop     ecx
		ret

! Delay after IO
delay:
		.word	0x00eb		! jmp $+2
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
		.word	0x80		!
		.word	512+gdt,0x9	! gdt base = 0X9xxxx

setup_sig1:	.word	SIG1
setup_sig2:	.word	SIG2

modelist:

! Make sure setup is big enough (4 sectors)
.org 2000

		.byte	0

.text
endtext:
.data
enddata:
.bss
endbss:
