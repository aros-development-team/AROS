/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2005,2006,2007,2008 Free Software Foundation, Inc.
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

#define ASM_FILE	1

#include <grub/symbol.h>
#include <grub/machine/memory.h>
#include <grub/cpu/linux.h>
#include <grub/cpu/kernel.h>
#include <multiboot.h>
#include <multiboot2.h>

/*
 * Note: GRUB is compiled with the options -mrtd and -mregparm=3.
 *       So the first three arguments are passed in %eax, %edx, and %ecx,
 *       respectively, and if a function has a fixed number of arguments
 *       and the number if greater than three, the function must return
 *       with "ret $N" where N is ((the number of arguments) - 3) * 4.
 */

	.file	"startup.S"
	.text
	.globl	start, _start

start:
_start:
	jmp codestart

	/*
	 *  This is a special data area at a fixed offset from the beginning.
	 */

	. = EXT_C(start) + GRUB_KERNEL_CPU_PREFIX

VARIABLE(grub_prefix)
	/* to be filled by grub-mkimage */

	/*
	 *  Leave some breathing room for the prefix.
	 */

	. = EXT_C(start) + GRUB_KERNEL_CPU_DATA_END

codestart:
	movl %eax, EXT_C(grub_ieee1275_entry_fn)
	jmp EXT_C(grub_main)

/*
 *  This call is special...  it never returns...  in fact it should simply
 *  hang at this point!
 */
FUNCTION(grub_stop)
	hlt
	jmp EXT_C(grub_stop)

/*
 *  prot_to_real and associated structures (but NOT real_to_prot, that is
 *  only needed for BIOS gates).
 */
#include "../realmode.S"

/*
 *  Routines needed by Linux and Multiboot loaders.
 */
#include "../loader.S"
