/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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
#include <multiboot.h>
#include <multiboot2.h>

	.p2align	2	/* force 4-byte alignment */

/*
 * This starts the multiboot kernel.
 */

VARIABLE(grub_multiboot_payload_size)
	.long	0
VARIABLE(grub_multiboot_payload_orig)
	.long	0
VARIABLE(grub_multiboot_payload_dest)
	.long	0
VARIABLE(grub_multiboot_payload_entry_offset)
	.long	0

/*
 * The relocators below understand the following parameters:
 * ecx:	Size of the block to be copied.
 * esi:	Where to copy from (always lowest address, even if we're relocating
 *      backwards).
 * edi:	Where to copy to (likewise).
 * edx:	Offset of the entry point (relative to the beginning of the block).
 */

VARIABLE(grub_multiboot_forward_relocator)
	/* Add entry offset.  */
	addl	%edi, %edx

	/* Forward copy.  */
	cld
	rep
	movsb

	jmp	*%edx
VARIABLE(grub_multiboot_forward_relocator_end)

VARIABLE(grub_multiboot_backward_relocator)
	/* Add entry offset (before %edi is mangled).  */
	addl	%edi, %edx

	/* Backward movsb is implicitly off-by-one.  compensate that.  */
	decl	%esi
	decl	%edi

	/* Backward copy.  */
	std
	addl	%ecx, %esi
	addl	%ecx, %edi
	rep
	movsb

	cld
	jmp	*%edx
VARIABLE(grub_multiboot_backward_relocator_end)

FUNCTION(grub_multiboot_real_boot)
	/* Push the entry address on the stack.  */
	pushl	%eax
	/* Move the address of the multiboot information structure to ebx.  */
	movl	%edx,%ebx

	/* Interrupts should be disabled.  */
	cli

	/* Where do we copy what from.  */
	movl	EXT_C(grub_multiboot_payload_size), %ecx
	movl	EXT_C(grub_multiboot_payload_orig), %esi
	movl	EXT_C(grub_multiboot_payload_dest), %edi
	movl	EXT_C(grub_multiboot_payload_entry_offset), %edx

	/* Move the magic value into eax.  */
	movl	$MULTIBOOT_MAGIC2, %eax

	/* Jump to the relocator.  */
	popl	%ebp
	jmp 	*%ebp

/*
 * This starts the multiboot 2 kernel.
 */

FUNCTION(grub_multiboot2_real_boot)
        /* Push the entry address on the stack.  */
        pushl   %eax
        /* Move the address of the multiboot information structure to ebx.  */
        movl    %edx,%ebx

        /* Interrupts should be disabled.  */
        cli

        /* Move the magic value into eax and jump to the kernel.  */
        movl    $MULTIBOOT2_BOOTLOADER_MAGIC,%eax
        popl    %ecx

        cld
        jmp     *%ecx
