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


/*
 * Note: These functions defined in this file may be called from C.
 *       Be careful of that you must not modify some registers. Quote
 *       from gcc-2.95.2/gcc/config/i386/i386.h:
	
   1 for registers not available across function calls.
   These must include the FIXED_REGISTERS and also any
   registers that can be used without being saved.
   The latter must include the registers where values are returned
   and the register where structure-value addresses are passed.
   Aside from that, you can include as many other registers as you like.

  ax,dx,cx,bx,si,di,bp,sp,st,st1,st2,st3,st4,st5,st6,st7,arg
{  1, 1, 1, 0, 0, 0, 0, 1, 1,  1,  1,  1,  1,  1,  1,  1,  1 }
 */

/*
 * Note: GRUB is compiled with the options -mrtd and -mregparm=3.
 *       So the first three arguments are passed in %eax, %edx, and %ecx,
 *       respectively, and if a function has a fixed number of arguments
 *       and the number if greater than three, the function must return
 *       with "ret $N" where N is ((the number of arguments) - 3) * 4.
 */

/*
 *  This is the area for all of the special variables.
 */

	.p2align	2	/* force 4-byte alignment */
	
/*
 * void grub_linux_boot_zimage (void)
 */
VARIABLE(grub_linux_prot_size)
	.long	0
VARIABLE(grub_linux_tmp_addr)
	.long	0
VARIABLE(grub_linux_real_addr)
	.long	0
VARIABLE(grub_linux_is_bzimage)
	.long	0
	
FUNCTION(grub_linux_boot)
	/* Must be done before zImage copy.  */
	call	EXT_C(grub_dl_unload_all)

	movl	EXT_C(grub_linux_is_bzimage), %ebx
	test	%ebx, %ebx
	jne bzimage

	/* copy the kernel */
	movl	EXT_C(grub_linux_prot_size), %ecx
	addl	$3, %ecx
	shrl	$2, %ecx
	movl	$GRUB_LINUX_BZIMAGE_ADDR, %esi
	movl	$GRUB_LINUX_ZIMAGE_ADDR, %edi
	cld
	rep
	movsl

bzimage:
	movl	EXT_C(grub_linux_real_addr), %ebx

	/* copy the real mode code */
	movl	EXT_C(grub_linux_tmp_addr), %esi
	movl	%ebx, %edi
	movl	$GRUB_LINUX_SETUP_MOVE_SIZE, %ecx
	cld
	rep
	movsb

	/* change %ebx to the segment address */
	shrl	$4, %ebx
	movl	%ebx, %eax
	addl	$0x20, %eax
	movw	%ax, linux_setup_seg

	/* XXX new stack pointer in safe area for calling functions */
	movl	$0x4000, %esp
	call	EXT_C(grub_stop_floppy)

	/* final setup for linux boot */
	call	prot_to_real
	.code16

	cli
	movw	%bx, %ss
	movw	$GRUB_LINUX_SETUP_STACK, %sp

	movw	%bx, %ds
	movw	%bx, %es
	movw	%bx, %fs
	movw	%bx, %gs

	/* ljmp */
	.byte	0xea
	.word	0
linux_setup_seg:
	.word	0
	.code32

		
/*
 * This starts the multiboot kernel.
 */

FUNCTION(grub_multiboot_real_boot)
	/* Push the entry address on the stack.  */
	pushl	%eax
	/* Move the address of the multiboot information structure to ebx.  */
	movl	%edx,%ebx
	
	/* Unload all modules and stop the floppy driver.  */
	call	EXT_C(grub_dl_unload_all)
	call	EXT_C(grub_stop_floppy)

	/* Interrupts should be disabled.  */
	cli
	
	/* Move the magic value into eax and jump to the kernel.  */
	movl	$MULTIBOOT_MAGIC2,%eax
	popl	%ecx
	jmp 	*%ecx

/*
 * This starts the multiboot 2 kernel.
 */

FUNCTION(grub_multiboot2_real_boot)
        /* Push the entry address on the stack.  */
        pushl   %eax
        /* Move the address of the multiboot information structure to ebx.  */
        movl    %edx,%ebx

        /* Unload all modules and stop the floppy driver.  */
        call    EXT_C(grub_dl_unload_all)
        call    EXT_C(grub_stop_floppy)

        /* Interrupts should be disabled.  */
        cli

        /* Move the magic value into eax and jump to the kernel.  */
        movl    $MULTIBOOT2_BOOTLOADER_MAGIC,%eax 
        popl    %ecx
        jmp     *%ecx
