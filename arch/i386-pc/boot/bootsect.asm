/*
    (C) 1997-98 AROS - The Amiga Research OS
    $Id$
 
    Desc: Bootblock for standalone AROS on PC
    Lang: English
*/

/*****************************************************************************

    FUNCTION
	It is loaded by BIOS at 0x7c00. Bootsect moves itself to 0x98000 and
	jumps there. It then loads setup after itself (0x98200) and the
	kernel at 0x00001000 using BIOS routines. It jumps then to setup
	where all hw settings are made. Setup calls then kernel. In that
	moment PC is already in 32-bit flat protected mode without memory
	protection or paging.

    NOTES
	Bootsect is similar to linux bootsect. Sorry for same comments. I did
	so because AROS is quite similar to linux, so similar loadres can be
	used.

    BUGS
	kernel size is limited to 604 kb. Hope it's enough :-))

******************************************************************************/

#include <aros/boot.h>

SYSSIZE	  = DEF_SYSSIZE
SETUPSECS = 2		! DO NOT change unless you know what are you doing!!!!
BOOTSEG	  = 0x07c0
INITSEG	  = DEF_INITSEG
SETUPSEG  = DEF_SETUPSEG
SYSSEG	  = DEF_SYSSEG

.text

.globl	_main

.org 0
		jmp	_main
		.ascii	"AROS"
_main:		mov	ax,#BOOTSEG
		mov	ds,ax
		mov	ax,#INITSEG
		mov	es,ax
		xor	si,si
		xor	di,di
		mov	cx,#256
		cld
		rep
		movsw
		jmpi	newseg,INITSEG

newseg:		mov	di,#0x4000-12	/* Length of bootsect+length of setup+
					** stacksize. 12 is disk parm size */
		mov	ds,ax
		mov	ss,ax
		mov	sp,di

/* We want to read kernel by whole tracks, so that it was loaded faster. We
** need to change parameter table for first floppy (the one we are booting
** from). So we'll copy them to RAM and change. See bootsect.S in
** /arch/i386/boot (linux sources) for more comments. */

/*
    Segment registers are set as follows:
    cs=ds=es=ss=INITSEG
    fs=0; gs is not used
*/

		mov	fs,cx		! After last loop cx was set to 0
		mov	bx,#0x78	! fs:bx is param table for floppy
		push	ds
		seg	fs
		lds	si,(bx)		! ds:si is source
		mov	cl,#6
		cld
		push	di		! di was set up to end of stack
		rep
		movsw
		pop	di
		pop	ds

		movb	4(di),#36	! Patch sector count
		seg	fs
		mov	(bx),di
		seg	fs
		mov	2(bx),es

/* Load the setup directly after bootsect. Note that es is already set up, and
   cx is set to 0 after last "rep movsw" instruction */

load_setup:	xor	ah,ah		! Reset FDC command
		xor	dl,dl
		int	0x13
		
		xor	dx,dx		! Drive 0 head 0
		mov	cl,#0x02	! Sector 2 cyl 0
		mov	bx,#0x0200	! es:bx=0x98200
		mov	ah,#2		! command - read sectors
		mov	al,#SETUPSECS	! sector count
		int	0x13
		
		jnc	setup_loaded

		push	ax
		call	print_crlf
		mov	bp,sp
		call	print_hex
		pop	ax
		
		decb	cnt1		! Try 5 times
		jne	load_setup
		
		call	print_fail	! Halt system if load fails
dead1:		jmp	dead1

/* Ok. We've just loaded our setup file. It will be used later */

setup_loaded:

! Now get disk drive parameters. This is done by trying to read a sector.
! Guess 36 sectors if sector 36 can be read, 18 sectors if sector 18 can be
! read, 15 sectors if sector 15 can be read, otherwise guess 9.

		mov	si,#disksizes
probe_loop:	lodsb			! Read sector number
		cbw			! Extend it to word
		mov	sectors,ax
		cmp	si,#disksizes+4
		jae	got_sectors	! If all else fails guess 9 sectors
		xchg	ax,cx		! cx will be track & cyl
		xor	dx,dx		! Drive 0 head 0
		xor	bl,bl
		mov	bh,#SETUPSECS
		inc	bh
		shl	bh,#1		! Address after setup
		mov	ax,#0x0201	! comm=2 (read) one sector
		int	0x13
		jc	probe_loop
		
got_sectors:	mov	ax,#INITSEG
		mov	es,ax

/* Print some message */

		mov	ah,#3		! Get cursor pos
		xor	bh,bh
		int	0x10
		
		mov	cx,#39		! 39 chars
		mov	bx,#0x0007	! page zero attrib normal
		mov	bp,#msg1
		mov	ax,#0x1301	! Write string, move cursor
		int	0x10

/* Now we want to load kernel */

		mov	ax,#SYSSEG
		mov	es,ax
		call	read_it
		call	kill_motor
		call	print_crlf

/* Everything is loaded now. It's time to jump to the setup */

		jmpi	0,SETUPSEG

/* This routine will read all kernel as fast as possible */

sread:
		.word	0	! sectors read of current track
head:
		.word	0	! current head
track:
		.word	0	! current track
ssize:
		.word	SYSSIZE	! kernel size in setcors

read_it:	mov	al,#SETUPSECS	! Skip bootsect and setup
		inc	al
		mov	sread,al
loop1_read:	mov	ax,sectors
		sub	ax,sread	! how many sectors on this track
		cmp	ax,ssize	! end of kernel? So don't read whole
		jb	loop2_read	! track
		mov	ax,ssize
loop2_read:	mov	dx,es		! Make sure DMA will not fail
		neg	dx
		and	dx,#0x0fff
		je	loop4_read
		shr	dx,#5
		cmp	ax,dx
		jb	loop4_read
		mov	ax,dx
loop4_read:	sub	ssize,ax
		movb	cnt1,#5		! in case of error try to load track
		call	read_track	! 5 times
		add	sread,ax
		mov	cx,ax
		shl	cx,#5
		mov	ax,es
		add	ax,cx
		mov	es,ax
		mov	ax,sread
		cmp	sectors,ax
		jne	loop3_read
		mov	sread,#0
		xor	head,#0x0100	! Change head
		jne	loop3_read
		inc	track		! Next track
		pusha
		mov	ax,#0x0e2e	! Put one dot after "Loading"
		mov	bx,#7
		int	0x10
		popa
loop3_read:	cmp	ssize,#0
		jne	loop1_read
		ret

read_track:	pusha
		mov	dx,track
		mov	cx,sread
		inc	cx
		mov	ch,dl
		mov	dx,head
		mov	ah,#2
		xor	bx,bx
		int	0x13
		popa
		jnc	read_ok
		decb	cnt1
		jne	read_track
		call	print_crlf
		call	print_fail
deadloop:	jmp	deadloop
read_ok:	ret

kill_motor:	push	dx
		mov	dx,#0x3f2
		xor	al,al
		out	dx,al
		pop	dx
		ret

print_crlf:	mov	ax,#0x0e0d	! 0e-write char 0d=CR
		int	0x10
		mov	al,#0x0a	! 0a=LF
		int	0x10
		ret

/* print_hex - print 4 digits, hex number pointed by ss:bp */

print_hex:	mov	cx,#4
		seg	ss
		mov	dx,(bp)
print_digit:	rol	dx,#4
		mov	ax,#0x0e0f	! 0f - mask for a nybble
		and	al,dl
		add	al,#0x90	! This 4 instructions convert al to
		daa			! ascii hex number
		adc	al,#0x40
		daa
		int	0x10
		loop	print_digit
		ret

print_fail:	mov	ah,#3
		xor	bh,bh
		int	0x10
		
		push	cs
		pop	es
		
		mov	cx,#34
		mov	bx,#0x0007
		mov	bp,#msg2
		mov	ax,#0x1301
		int	0x10
		ret

disksizes:
		.byte	36,18,15,9

sectors:
		.word	0	! sectors per track
cnt1:
		.byte	5
msg2:
		.byte	13,10
		.ascii	"Fatal read error. System halted."
msg1:
		.byte	13,10
		.ascii	"AROS - The Amiga Research OS"
		.byte	13,10
		.ascii	"Loading"

.org 510
		.word	$AA55
