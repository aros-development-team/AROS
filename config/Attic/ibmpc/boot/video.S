/*
    (C) 1997-98 AROS - The Amiga Research OS
    $Id$
    
    Desc: Video init for standalone AROS on PC
    Lang: English
*/

/*****************************************************************************

    FUNCTION
	This pice of code detect type of gfx card you have installed and turns
	mode 80x30 on if you have VGA or better.

	Video params (cursor position and other stuff) removed in 1.3
	Who have used it? Nobody i think...

    BUGS
	Incompleted initialization

*****************************************************************************/

#define VGA_INDICATOR	0x17

! This function is called from setup.S
! DS points to bootsector

video:		push	ds
		push	ds
		pop	fs		! fs contains original ds
		push	cs
		pop	ds		! ds=cs
		push	cs
		pop	es		! es=cs
		xor	ax,ax
		mov	gs,ax		! gs=0
		cld
		call	basic_detect	! Test video adapter
		
		call	mode_set
		
		pop	ds
		ret

!
! Detect installed video card
!

basic_detect:	seg	fs
		movb	(VGA_INDICATOR),#0
		
		mov	ah,#0x12
		mov	bl,#0x10
		int	0x10

		cmp	bl,#0x10	! This is MDA/CGA/HGA card
		je	basret
		incb	[adapter]
		
		mov	ax,#0x1a00	! Check for EGA/VGA discrimination
		int	0x10
		cmp	al,#0x1a	! 0x1a means VGA
		jne	basret
		seg	fs
		incb	(VGA_INDICATOR)
		incb	[adapter]
basret:		ret

! Set 80x30 mode. 480 scanlines and 16 pixel font
! If you haven't VGA video.S halts.

mode_set:	cmpb	[adapter],#0	! Hope we have 80x25 set
		je	no_vga
		mov	al,#0x03
		int	0x10
		mov	dx,#0x3cc	! Get CRTC port
		in	al,dx
		mov	dl,#0xd4	! Valid for color mode
		ror	al,#1		! Do we have color
		jc	setm
		mov	dl,#0xb4	! No, set mono
setm:		mov	ax,#0x0c11	! Vertical sync end (unlocks CR0-CR7)
		call	outidx
		mov	ax,#0x0b06	! Vertical total
		call	outidx
		mov	ax,#0x3e07	! Overflow
		call	outidx	
		mov	ax,#0xea10	! Vertical sync start
		call	outidx
		mov	ax,#0xdf12	! Vertical display end
		call	outidx
		mov	ax,#0xe715	! Vertical blank start
		call	outidx
		mov	ax,#0x0416	! Vertical blank end
		call	outidx
		push	dx
		mov	dl,#0xcc	! Misc output register (read)
		in	al,dx
		mov	dx,#0xc2	! (write)
		and	al,#0x0d	! Preserve clock select and color bits
		or	al,#0xe2	! Set correct sync polarity
		out	dx,al
		pop	dx
		stc			! Done
		ret

! Out indexed register. al contains register index, ah contains data

outidx:		out	dx,al		! Out index
		push	ax
		inc	dx
		xchg	ah,al
		out	dx,al		! Out data
		dec	dx
		pop	ax
		ret

! Write sad message in case you haven't VGA or better

no_vga:		mov	si,#failmsg
		call	prtstr
no_vga_hlt:	jmp	no_vga_hlt

adapter:
		.byte	0
failmsg:
		.byte	13,10
		.ascii	"Sorry - AROS needs VGA or better graphics driver."
		.byte	13,10,0
