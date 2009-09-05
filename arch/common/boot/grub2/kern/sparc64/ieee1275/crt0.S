/* crt0.S - Startup code for the Sparc64.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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
#include <grub/symbol.h>
#include <grub/machine/kernel.h>

	.text
	.align	4
	.globl	_start
_start:
	ba	codestart
	 nop

	. = EXT_C(_start) + GRUB_KERNEL_MACHINE_TOTAL_MODULE_SIZE

VARIABLE(grub_total_module_size)
	.word	0
VARIABLE(grub_kernel_image_size)
	.word	0
VARIABLE(grub_compressed_size)
	.word	0
VARIABLE(grub_prefix)
	/* to be filled by grub-mkimage */

	/*
	 *  Leave some breathing room for the prefix.
	 */

	. = EXT_C(_start) + GRUB_KERNEL_MACHINE_DATA_END

codestart:
	/* Copy the modules past the end of the kernel image.
	 * They are currently sitting in the BSS.
	 */
	sethi	%hi(__bss_start), %o2
	or	%o2, %lo(__bss_start), %o2
	sethi	%hi(_end), %o3
	or	%o3, %lo(_end), %o3
	sethi	%hi(grub_total_module_size), %o4
	lduw	[%o4 + %lo(grub_total_module_size)], %o4
1:	lduw	[%o2], %o5
	stw	%o5, [%o3]
	subcc	%o4, 4, %o4
	add	%o2, 4, %o2
	bne,pt	%icc, 1b
	 add	%o3, 4, %o3

	/* Now it's safe to clear out the BSS.  */
	sethi	%hi(__bss_start), %o2
	or	%o2, %lo(__bss_start), %o2
	sethi	%hi(_end), %o3
	or	%o3, %lo(_end), %o3
1:	stx	%g0, [%o2]
	add	%o2, 8, %o2
	cmp	%o2, %o3
	blt,pt	%xcc, 1b
	 nop
	sethi	%hi(grub_ieee1275_entry_fn), %o2
	stx	%o0, [%o2 + %lo(grub_ieee1275_entry_fn)]
	call	grub_main
	 nop
1:	ba,a	1b
