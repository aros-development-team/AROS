*
* :ts=8
*
* SMB file system wrapper for AmigaOS, using the AmiTCP V3 API
*
* Copyright (C) 2016 by Olaf `Olsen' Barthel <obarthel -at- gmx -dot- net>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*

	include	"exec/macros.i"

	section	text,code

_LVOStackSwap equ -732

	xref	_SysBase

	xdef	_swap_stack_and_call

_swap_stack_and_call:

	movem.l	d2/a2/a3/a6,-(sp)

	move.l	_SysBase,a6

	move.l	20(sp),a2
	move.l	24(sp),a3

	move.l	a2,a0
	jsr	_LVOStackSwap(a6)

	jsr	(a3)
	move.l	d0,d2

	move.l	a2,a0
	jsr	_LVOStackSwap(a6)

	move.l	d2,d0

	movem.l	(sp)+,d2/a2/a3/a6
	rts

	end
