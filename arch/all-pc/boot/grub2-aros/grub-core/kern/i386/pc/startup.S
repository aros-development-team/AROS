/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2005,2006,2007,2008,2009 Free Software Foundation, Inc.
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
 *       and the number is greater than three, the function must return
 *       with "ret $N" where N is ((the number of arguments) - 3) * 4.
 */

#include <config.h>
#include <grub/symbol.h>
#include <grub/boot.h>
#include <grub/machine/boot.h>
#include <grub/machine/memory.h>
#include <grub/machine/console.h>
#include <grub/cpu/linux.h>
#include <grub/machine/kernel.h>
#include <grub/term.h>
#include <multiboot.h>
#include <multiboot2.h>

#define ABS(x)	((x) - LOCAL (base) + GRUB_BOOT_MACHINE_KERNEL_ADDR + 0x200)

	.file	"startup.S"

	.text

	/* Tell GAS to generate 16-bit instructions so that this code works
	   in real mode. */
	.code16

	.globl	start, _start
start:
_start:
LOCAL (base):
	/*
	 *  Guarantee that "main" is loaded at 0x0:0x8200.
	 */
#ifdef __APPLE__
	ljmp $0, $(ABS(LOCAL (codestart)) - 0x10000)
#else
	ljmp $0, $ABS(LOCAL (codestart))
#endif
	/*
	 *  Compatibility version number
	 *
	 *  These MUST be at byte offset 6 and 7 of the executable
	 *  DO NOT MOVE !!!
	 */
	. = _start + 0x6
	.byte	GRUB_BOOT_VERSION_MAJOR, GRUB_BOOT_VERSION_MINOR

	/*
	 *  This is a special data area 8 bytes from the beginning.
	 */

	. = _start + 0x8

VARIABLE(grub_total_module_size)
	.long	0
VARIABLE(grub_kernel_image_size)
	.long	0
VARIABLE(grub_compressed_size)
	.long	0
VARIABLE(grub_install_dos_part)
	.long	0xFFFFFFFF
VARIABLE(grub_install_bsd_part)
	.long	0xFFFFFFFF
reed_solomon_redundancy:
	.long	0

#ifdef APPLE_CC
bss_start:
	.long 0
bss_end:
	.long 0
#endif
/*
 *  This is the area for all of the special variables.
 */

VARIABLE(grub_boot_drive)
	.byte	0

/* the real mode code continues... */
LOCAL (codestart):
	cli		/* we're not safe here! */

	/* set up %ds, %ss, and %es */
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %ss
	movw	%ax, %es

	/* set up the real mode/BIOS stack */
	movl	$GRUB_MEMORY_MACHINE_REAL_STACK, %ebp
	movl	%ebp, %esp

	sti		/* we're safe again */

	/* save the boot drive */
	ADDR32	movb	%dl, EXT_C(grub_boot_drive)

	/* reset disk system (%ah = 0) */
	int	$0x13

	/* transition to protected mode */
	DATA32	call real_to_prot

	/* The ".code32" directive takes GAS out of 16-bit mode. */
	.code32

	incl	%eax
	call	grub_gate_a20

	movl	EXT_C(grub_compressed_size), %edx
	addl    $(GRUB_KERNEL_MACHINE_RAW_SIZE - GRUB_KERNEL_I386_PC_NO_REED_SOLOMON_PART), %edx
	movl    reed_solomon_redundancy, %ecx
	leal    _start + GRUB_KERNEL_I386_PC_NO_REED_SOLOMON_PART, %eax
	call    EXT_C (grub_reed_solomon_recover)
	jmp post_reed_solomon

#include <rs_decoder.S>

	.text

	. = _start + GRUB_KERNEL_I386_PC_NO_REED_SOLOMON_PART
/*
 * Support for booting GRUB from a Multiboot boot loader (e.g. GRUB itself).
 * This uses the a.out kludge to load raw binary to the area starting at 1MB,
 * and relocates itself after loaded.
 */
	.p2align	2	/* force 4-byte alignment */
multiboot_header:
	/* magic */
	.long	0x1BADB002
	/* flags */
	.long	(1 << 16)
	/* checksum */
	.long	-0x1BADB002 - (1 << 16)
	/* header addr */
	.long	multiboot_header - _start + 0x100000 + 0x200
	/* load addr */
	.long	0x100000
	/* load end addr */
	.long	0
	/* bss end addr */
	.long	0
	/* entry addr */
	.long	multiboot_entry - _start + 0x100000 + 0x200

multiboot_entry:
	.code32
	/* obtain the boot device */
	movl	12(%ebx), %edx

	movl	$GRUB_MEMORY_MACHINE_PROT_STACK, %ebp
	movl	%ebp, %esp

	/* relocate the code */
	movl	$(GRUB_KERNEL_MACHINE_RAW_SIZE + 0x200), %ecx
	addl	EXT_C(grub_compressed_size) - _start + 0x100000 + 0x200, %ecx
	movl	$0x100000, %esi
	movl	$GRUB_BOOT_MACHINE_KERNEL_ADDR, %edi
	cld
	rep
	movsb
	/* jump to the real address */
	movl	$multiboot_trampoline, %eax
	jmp	*%eax

multiboot_trampoline:
	/* fill the boot information */
	movl	%edx, %eax
	shrl	$8, %eax
	xorl	%ebx, %ebx
	cmpb	$0xFF, %ah
	je	1f
	movb	%ah, %bl
	movl	%ebx, EXT_C(grub_install_dos_part)
1:
	cmpb	$0xFF, %al
	je	2f
	movb	%al, %bl
	movl	%ebx, EXT_C(grub_install_bsd_part)
2:
	shrl	$24, %edx
        movb    $0xFF, %dh
	/* enter the usual booting */
	call	prot_to_real
	jmp     LOCAL (codestart)

post_reed_solomon:

#ifdef ENABLE_LZMA
	movl	$GRUB_MEMORY_MACHINE_DECOMPRESSION_ADDR, %edi
	movl	$(_start + GRUB_KERNEL_MACHINE_RAW_SIZE), %esi
	pushl	%edi
	pushl	%esi
	movl	EXT_C(grub_kernel_image_size), %ecx
	addl	EXT_C(grub_total_module_size), %ecx
	subl	$GRUB_KERNEL_MACHINE_RAW_SIZE, %ecx
	pushl	%ecx
	leal	(%edi, %ecx), %ebx
	call	_LzmaDecodeA
	/* _LzmaDecodeA clears DF, so no need to run cld */
	popl	%ecx
	popl	%edi
	popl	%esi
#endif

	/* copy back the decompressed part (except the modules) */
	subl	EXT_C(grub_total_module_size), %ecx
	rep
	movsb

#if 0
	/* copy modules before cleaning out the bss */
	movl	EXT_C(grub_total_module_size), %ecx
	movl	EXT_C(grub_kernel_image_size), %esi
	addl	%ecx, %esi
	addl	$_start, %esi
	decl	%esi
	movl	$END_SYMBOL, %edi
	addl	%ecx, %edi
	decl	%edi
	std
	rep
	movsb
#endif

#ifdef APPLE_CC
	/* clean out the bss */
	bss_start_abs = ABS (bss_start)
	bss_end_abs = ABS (bss_end)

	movl    bss_start_abs, %edi

	/* compute the bss length */
	movl	bss_end_abs, %ecx
	subl	%edi, %ecx
#else
	/* clean out the bss */
	movl	$BSS_START_SYMBOL, %edi

	/* compute the bss length */
	movl	$END_SYMBOL, %ecx
	subl	%edi, %ecx
#endif

	/* clean out */
	xorl	%eax, %eax
	cld
	rep
	stosb

	/*
	 *  Call the start of main body of C code.
	 */
	call EXT_C(grub_main)

#include "../realmode.S"

/*
 * grub_gate_a20(int on)
 *
 * Gate address-line 20 for high memory.
 *
 * This routine is probably overconservative in what it does, but so what?
 *
 * It also eats any keystrokes in the keyboard buffer.  :-(
 */

grub_gate_a20:	
	movl	%eax, %edx

gate_a20_test_current_state:
	/* first of all, test if already in a good state */
	call	gate_a20_check_state
	cmpb	%al, %dl
	jnz	gate_a20_try_bios
	ret

gate_a20_try_bios:
	/* second, try a BIOS call */
	pushl	%ebp
	call	prot_to_real

	.code16
	movw	$0x2400, %ax
	testb	%dl, %dl
	jz	1f
	incw	%ax
1:	int	$0x15

	DATA32	call	real_to_prot
	.code32

	popl	%ebp
	call	gate_a20_check_state
	cmpb	%al, %dl
	jnz	gate_a20_try_system_control_port_a
	ret

gate_a20_try_system_control_port_a:
	/*
	 * In macbook, the keyboard test would hang the machine, so we move
	 * this forward.
	 */
	/* fourth, try the system control port A */
	inb	$0x92
	andb	$(~0x03), %al
	testb	%dl, %dl
	jz	6f
	orb	$0x02, %al
6:	outb	$0x92

	/* When turning off Gate A20, do not check the state strictly,
	   because a failure is not fatal usually, and Gate A20 is always
	   on some modern machines.  */
	testb	%dl, %dl
	jz	7f
	call	gate_a20_check_state
	cmpb	%al, %dl
	jnz	gate_a20_try_keyboard_controller
7:	ret

gate_a20_flush_keyboard_buffer:
	inb	$0x64
	andb	$0x02, %al
	jnz	gate_a20_flush_keyboard_buffer
2:
	inb	$0x64
	andb	$0x01, %al
	jz	3f
	inb	$0x60
	jmp	2b
3:
	ret

gate_a20_try_keyboard_controller:
	/* third, try the keyboard controller */
	call    gate_a20_flush_keyboard_buffer

	movb	$0xd1, %al
	outb	$0x64
4:
	inb	$0x64
	andb	$0x02, %al
	jnz	4b

	movb	$0xdd, %al
	testb	%dl, %dl
	jz	5f
	orb	$0x02, %al
5:	outb	$0x60
	call    gate_a20_flush_keyboard_buffer

	/* output a dummy command (USB keyboard hack) */
	movb	$0xff, %al
	outb	$0x64
	call    gate_a20_flush_keyboard_buffer

	call	gate_a20_check_state
	cmpb	%al, %dl
	/* everything failed, so restart from the beginning */
	jnz	gate_a20_try_bios
	ret

gate_a20_check_state:
	/* iterate the checking for a while */
	movl	$100, %ecx
1:
	call	3f
	cmpb	%al, %dl
	jz	2f
	loop	1b
2:
	ret
3:
	pushl	%ebx
	pushl	%ecx
	xorl	%eax, %eax
	/* compare the byte at 0x8000 with that at 0x108000 */
	movl	$GRUB_BOOT_MACHINE_KERNEL_ADDR, %ebx
	pushl	%ebx
	/* save the original byte in CL */
	movb	(%ebx), %cl
	/* store the value at 0x108000 in AL */
	addl	$0x100000, %ebx
	movb	(%ebx), %al
	/* try to set one less value at 0x8000 */
	popl	%ebx
	movb	%al, %ch
	decb	%ch
	movb	%ch, (%ebx)
	/* serialize */
	outb	%al, $0x80
	outb	%al, $0x80
	/* obtain the value at 0x108000 in CH */
	pushl	%ebx
	addl	$0x100000, %ebx
	movb	(%ebx), %ch
	/* this result is 1 if A20 is on or 0 if it is off */
	subb	%ch, %al
	xorb	$1, %al
	/* restore the original */
	popl	%ebx
	movb	%cl, (%ebx)
	popl	%ecx
	popl	%ebx
	ret

#ifdef ENABLE_LZMA
#include "lzma_decode.S"
#endif

/*
 * The code beyond this point is compressed.  Assert that the uncompressed
 * code fits GRUB_KERNEL_MACHINE_RAW_SIZE.
 */
	. = _start + GRUB_KERNEL_MACHINE_RAW_SIZE

	. = _start + GRUB_KERNEL_MACHINE_PREFIX
VARIABLE(grub_prefix)
	/* to be filled by grub-mkimage */

	/*
	 *  Leave some breathing room for the prefix.
	 */
	. = _start + GRUB_KERNEL_MACHINE_PREFIX_END



/*
 * grub_exit()
 *
 * Exit the system.
 */
FUNCTION(grub_exit)
	call	prot_to_real
	.code16
	/* Tell the BIOS a boot failure. If this does not work, reboot.  */
	int	$0x18
	jmp	cold_reboot
	.code32

/*
 *  void grub_chainloader_real_boot (int drive, void *part_addr)
 *
 *  This starts another boot loader.
 */

FUNCTION(grub_chainloader_real_boot)
	pushl	%edx
	pushl	%eax

	/* Turn off Gate A20 */
	xorl	%eax, %eax
	call	grub_gate_a20

	/* set up to pass boot drive */
	popl	%edx

	/* ESI must point to a partition table entry */
	popl	%esi

	call	prot_to_real
	.code16
	ljmp	$0, $GRUB_MEMORY_MACHINE_BOOT_LOADER_ADDR
	.code32

/*
 * void grub_console_putchar (int c)
 *
 * Put the character C on the console. Because GRUB wants to write a
 * character with an attribute, this implementation is a bit tricky.
 * If C is a control character (CR, LF, BEL, BS), use INT 10, AH = 0Eh
 * (TELETYPE OUTPUT). Otherwise, save the original position, put a space,
 * save the current position, restore the original position, write the
 * character and the attribute, and restore the current position.
 *
 * The reason why this is so complicated is that there is no easy way to
 * get the height of the screen, and the TELETYPE OUTPUT BIOS call doesn't
 * support setting a background attribute.
 */
FUNCTION(grub_console_putchar)
	/* Retrieve the base character.  */
	movl	0(%edx), %edx
	pusha
	movb	EXT_C(grub_console_cur_color), %bl

	call	prot_to_real
	.code16
	movb	%dl, %al
	xorb	%bh, %bh

	/* use teletype output if control character */
	cmpb	$0x7, %al
	je	1f
	cmpb	$0x8, %al
	je	1f
	cmpb	$0xa, %al
	je	1f
	cmpb	$0xd, %al
	je	1f

	/* save the character and the attribute on the stack */
	pushw	%ax
	pushw	%bx

	/* get the current position */
	movb	$0x3, %ah
	int	$0x10

	/* check the column with the width */
	cmpb	$79, %dl
	jl	2f

	/* print CR and LF, if next write will exceed the width */
	movw	$0x0e0d, %ax
	int	$0x10
	movb	$0x0a, %al
	int	$0x10

	/* get the current position */
	movb	$0x3, %ah
	int	$0x10

2:
	/* restore the character and the attribute */
	popw	%bx
	popw	%ax

	/* write the character with the attribute */
	movb	$0x9, %ah
	movw	$1, %cx
	int	$0x10

	/* move the cursor forward */
	incb	%dl
	movb	$0x2, %ah
	int	$0x10

	jmp	3f

1:	movw	$1, %bx
	movb	$0xe, %ah
	int	$0x10

3:	DATA32	call	real_to_prot
	.code32

	popa
	ret


LOCAL(bypass_table):
	.word 0x011b, 0x0f00 | '\t', 0x0e00 | '\b', 0x1c00 | '\r'
	.word 0x1c00 | '\n'
LOCAL(bypass_table_end):

/*
 * int grub_console_getkey (void)
 *	if there is a character pending, return it; otherwise return -1
 * BIOS call "INT 16H Function 01H" to check whether a character is pending
 *	Call with	%ah = 0x1
 *	Return:
 *		If key waiting to be input:
 *			%ah = keyboard scan code
 *			%al = ASCII character
 *			Zero flag = clear
 *		else
 *			Zero flag = set
 * BIOS call "INT 16H Function 00H" to read character from keyboard
 *	Call with	%ah = 0x0
 *	Return:		%ah = keyboard scan code
 *			%al = ASCII character
 */

FUNCTION(grub_console_getkey)
	pushl	%ebp
	pushl   %edi

	call	prot_to_real
	.code16

	/*
	 * Due to a bug in apple's bootcamp implementation, INT 16/AH = 0 would
	 * cause the machine to hang at the second keystroke. However, we can
	 * work around this problem by ensuring the presence of keystroke with
	 * INT 16/AH = 1 before calling INT 16/AH = 0.
	 */

	movb	$1, %ah
	int	$0x16
	jz	notpending

	movb	$0, %ah
	int	$0x16

	xorl    %edx, %edx
	movw	%ax, %dx		/* real_to_prot uses %eax */

	DATA32	call	real_to_prot
	.code32

	movl    $0xff, %eax
	testl   %eax, %edx
	jz      1f

	andl	%edx, %eax
	cmpl    $0x20, %eax
	jae     2f
	movl	%edx, %eax
	leal    LOCAL(bypass_table), %edi
	movl    $((LOCAL(bypass_table_end) - LOCAL(bypass_table)) >> 1), %ecx
	repne scasw 
	jz      3f

	andl	$0xff, %eax
	addl    $(('a' - 1) | GRUB_TERM_CTRL), %eax
	jmp     2f
3:
	andl    $0xff, %eax
	jmp 2f

1:	movl    %edx, %eax
	shrl    $8, %eax
	orl     $GRUB_TERM_EXTENDED, %eax
2:
	popl    %edi
	popl	%ebp
	ret

notpending:	
	.code16
	DATA32	call	real_to_prot
	.code32
#if GRUB_TERM_NO_KEY != 0
#error Fix this asm code
#endif
	jmp 2b


/*
 * grub_uint16_t grub_console_getxy (void)
 * BIOS call "INT 10H Function 03h" to get cursor position
 *	Call with	%ah = 0x03
 *			%bh = page
 *      Returns         %ch = starting scan line
 *                      %cl = ending scan line
 *                      %dh = row (0 is top)
 *                      %dl = column (0 is left)
 */


FUNCTION(grub_console_getxy)
	pushl	%ebp
	pushl	%ebx                    /* save EBX */

	call	prot_to_real
	.code16

        xorb	%bh, %bh                /* set page to 0 */
	movb	$0x3, %ah
	int	$0x10			/* get cursor position */

	DATA32	call	real_to_prot
	.code32

	movb	%dl, %ah
	movb	%dh, %al

	popl	%ebx
	popl	%ebp
	ret


/*
 * void grub_console_gotoxy(grub_uint8_t x, grub_uint8_t y)
 * BIOS call "INT 10H Function 02h" to set cursor position
 *	Call with	%ah = 0x02
 *			%bh = page
 *                      %dh = row (0 is top)
 *                      %dl = column (0 is left)
 */


FUNCTION(grub_console_gotoxy)
	pushl	%ebp
	pushl	%ebx                    /* save EBX */

	movb	%cl, %dh	/* %dh = y */
	/* %dl = x */

	call	prot_to_real
	.code16

        xorb	%bh, %bh                /* set page to 0 */
	movb	$0x2, %ah
	int	$0x10			/* set cursor position */

	DATA32	call	real_to_prot
	.code32

	popl	%ebx
	popl	%ebp
	ret


/*
 * void grub_console_cls (void)
 * BIOS call "INT 10H Function 09h" to write character and attribute
 *	Call with	%ah = 0x09
 *                      %al = (character)
 *                      %bh = (page number)
 *                      %bl = (attribute)
 *                      %cx = (number of times)
 */

FUNCTION(grub_console_cls)
	pushl	%ebp
	pushl	%ebx                    /* save EBX */

	call	prot_to_real
	.code16

	/* move the cursor to the beginning */
	movb	$0x02, %ah
	xorb	%bh, %bh
	xorw	%dx, %dx
	int	$0x10

	/* write spaces to the entire screen */
	movw	$0x0920, %ax
	movw	$0x07, %bx
	movw	$(80 * 25), %cx
        int	$0x10

	/* move back the cursor */
	movb	$0x02, %ah
	int	$0x10

	DATA32	call	real_to_prot
	.code32

	popl	%ebx
	popl	%ebp
	ret


/*
 * void grub_console_setcursor (int on)
 * BIOS call "INT 10H Function 01h" to set cursor type
 *      Call with       %ah = 0x01
 *                      %ch = cursor starting scanline
 *                      %cl = cursor ending scanline
 */

console_cursor_state:
	.byte	1
console_cursor_shape:
	.word	0

FUNCTION(grub_console_setcursor)
	pushl	%ebp
	pushl	%ebx

	/* push ON */
	pushl	%edx

	/* check if the standard cursor shape has already been saved */
	movw	console_cursor_shape, %ax
	testw	%ax, %ax
	jne	1f

	call	prot_to_real
	.code16

	movb	$0x03, %ah
	xorb	%bh, %bh
	int	$0x10

	DATA32	call	real_to_prot
	.code32

	cmp     %cl, %ch
	jb      3f
	movw    $0x0d0e, %cx
3:	
	movw	%cx, console_cursor_shape
1:
	/* set %cx to the designated cursor shape */
	movw	$0x2000, %cx
	popl	%eax
	testl	%eax, %eax
	jz	2f
	movw	console_cursor_shape, %cx
2:
	call	prot_to_real
	.code16

	movb    $0x1, %ah
	int     $0x10

	DATA32	call	real_to_prot
	.code32

	popl	%ebx
	popl	%ebp
	ret

/*
 * grub_get_rtc()
 *	return the real time in ticks, of which there are about
 *	18-20 per second
 */
FUNCTION(grub_get_rtc)
	pushl	%ebp

	call	prot_to_real	/* enter real mode */
	.code16

	/* %ax is already zero */
        int	$0x1a

	DATA32	call	real_to_prot
	.code32

	movl	%ecx, %eax
	shll	$16, %eax
	movw	%dx, %ax

	popl	%ebp
	ret

/*
 * int grub_pxe_call (int func, void* data, grub_uint32_t pxe_rm_entry);
 */
FUNCTION(grub_pxe_call)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%edi
	pushl	%ebx

	movl	%ecx, %ebx
	movl	%eax, %ecx
	movl	%edx, %eax
	andl	$0xF, %eax
	shrl	$4, %edx
	shll	$16, %edx
	addl	%eax, %edx

	call    prot_to_real
	.code16

	pushl	%ebx
	pushl	%edx
	pushw	%cx
	movw	%sp, %bx
	lcall	*%ss:6(%bx)
	cld
	addw	$10, %sp
	movw	%ax, %cx

	DATA32  call	real_to_prot
	.code32

	movzwl	%cx, %eax

	popl	%ebx
	popl	%edi
	popl	%esi
	popl	%ebp
	ret

FUNCTION(grub_bios_interrupt)
	pushl    %ebp
	pushl    %ecx
	pushl    %eax
	pushl    %ebx
	pushl    %esi
	pushl    %edi	
	pushl    %edx
	
	movb     %al, intno
	movl	 (%edx), %eax
	movl	 %eax, LOCAL(bios_register_eax)
	movw	 4(%edx), %ax
	movw	 %ax, LOCAL(bios_register_es)
	movw	 6(%edx), %ax
	movw	 %ax, LOCAL(bios_register_ds)
	movw	 8(%edx), %ax
	movw	 %ax, LOCAL(bios_register_flags)

	movl 	12(%edx), %ebx
	movl 	16(%edx), %ecx
	movl 	20(%edx), %edi
	movl 	24(%edx), %esi
	movl 	28(%edx), %edx

	call    prot_to_real
	.code16

	mov	%ds, %ax
	push	%ax

	/* movw imm16, %ax*/
	.byte	0xb8
LOCAL(bios_register_es):
	.short 	0
	movw	%ax, %es
	/* movw imm16, %ax*/
	.byte	0xb8
LOCAL(bios_register_ds):
	.short 	0
	movw	%ax, %ds

	/* movw imm16, %ax*/
	.byte	0xb8
LOCAL(bios_register_flags):
	.short 	0
	push	%ax
	popf

	/* movl imm32, %eax*/
	.byte	0x66, 0xb8
LOCAL(bios_register_eax):
	.long 	0
	
	/* int imm8.  */
	.byte   0xcd
intno:	
	.byte   0

	movl 	%eax, %cs:LOCAL(bios_register_eax)
	movw	%ds, %ax
	movw 	%ax, %cs:LOCAL(bios_register_ds)
	pop 	%ax
	mov	%ax, %ds
	pushf
	pop	%ax
	movw	%ax, LOCAL(bios_register_flags)
	mov 	%es, %ax
	movw	%ax, LOCAL(bios_register_es)

	DATA32  call	real_to_prot
	.code32

	popl    %eax

	movl 	%ebx, 12(%eax)
	movl 	%ecx, 16(%eax)
	movl 	%edi, 20(%eax)
	movl 	%esi, 24(%eax)
	movl 	%edx, 28(%eax)

	movl     %eax, %edx

	movl	 LOCAL(bios_register_eax), %eax
	movl	 %eax, (%edx)
	movw	 LOCAL(bios_register_es), %ax
	movw	 %ax, 4(%edx)
	movw	 LOCAL(bios_register_ds), %ax
	movw	 %ax, 6(%edx)
	movw	 LOCAL(bios_register_flags), %ax
	movw	 %ax, 8(%edx)

	popl 	%edi
	popl 	%esi
	popl    %ebx
	popl    %eax
	popl    %ecx
	popl    %ebp
	ret
