/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000,2005,2007,2008   Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

	.file	"pxeboot.S"
	.text

	/* Start with the prehistoric environment... */
	.code16
	
	/* Let's go */
.globl _start; _start:

        /* Root drive will default to boot drive */
        movb	$0xFF, %dh
        movb	$0x7F, %dl
        
	/* Jump to the real world */
	ljmp	$0, $0x8200

	/* This region is a junk. Do you say that this is wasteful?
	   But I like that the memory layout of the body is consistent
	   among different kernels rather than scamping just for 1.5KB. */
	. = _start + 0x8200 - 0x7C00 - 0x200 - 1
	.byte	0
