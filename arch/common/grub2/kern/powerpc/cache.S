/* cache.S - Flush the processor cache for a specific region.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2004  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define CACHE_LINE_BYTES 32

	         .text

	         .align 2
	         .globl grub_arch_sync_caches
grub_arch_sync_caches:
	/* Force the dcache lines to memory.  */
	li 5, 0
1:	dcbst 5, 3
	addi 5, 5, CACHE_LINE_BYTES
	cmpw 5, 4
	blt 1b
	sync            /* Force all dcbsts to complete.  */

	/* Invalidate the icache lines.  */
	li 5, 0
1:	icbi 5, 3
	addi 5, 5, CACHE_LINE_BYTES
	cmpw 5, 4
	blt 1b
	sync            /* Force all icbis to complete.  */
	isync           /* Discard partially executed instructions that were
		           loaded from the invalid icache.  */
	blr
