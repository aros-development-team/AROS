/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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

#define MODE_RDRW 2
#define FLAGS_NONE 0
#define SYSCALL_ARCH 165
#define SYSCALL_OPEN 5
#define SYSCALL_WRITE 4
#define SYSCALL_RESET 55
#define SYSCALL_EXIT 1
#define SYSCALL_ARCH_IOPL	4
#define SYSCALL_FSYNC 95

#define RESET_NOSYNC 0x4
#define RESET_HALT 0x8
#define RESET_POWEROFF 0x4000
#define SHUTDOWN_PORT 0x8900

	.section ".init", "ax"
	.global start,_start
start:
_start:
	/* open.  */
	movq $SYSCALL_OPEN, %rax
	leaq device, %rdi
	movq $MODE_RDRW, %rsi
	movq $FLAGS_NONE, %rdx
	syscall
	movq %rax, %rdi

	/* write.  */
	leaq message, %rsi
	movq $SYSCALL_WRITE, %rax
	movq $(messageend - message), %rdx
	syscall

	/* fsync.  */
	movq $SYSCALL_FSYNC, %rax
	syscall

	/* IOPL.  */
	movq $SYSCALL_ARCH, %rax
	movq $SYSCALL_ARCH_IOPL, %rdi
	leaq iopl_arg, %rsi
	syscall

	movw $SHUTDOWN_PORT, %dx
	movb $'S', %al
	outb %al, %dx
	movb $'h', %al
	outb %al, %dx
	movb $'u', %al
	outb %al, %dx
	movb $'t', %al
	outb %al, %dx
	movb $'d', %al
	outb %al, %dx
	movb $'o', %al
	outb %al, %dx
	movb $'w', %al
	outb %al, %dx
	movb $'n', %al
	outb %al, %dx
	
	/* shutdown.  */
	movq $SYSCALL_RESET, %rax
	movq $(RESET_POWEROFF|RESET_HALT|RESET_NOSYNC), %rdi
	syscall

	/* exit (1). Shouldn't be reached.  */
	movq $SYSCALL_EXIT, %rax
	movq $1, %rdi
	syscall
device:
	.ascii "/dev/console"
	.byte 0
message:
	.ascii "Boot Test Passed Successfully\n" SUCCESSFUL_BOOT_STRING "\n"
messageend:
iopl_arg:
	.long SHUTDOWN_PORT
	.long 1
	.long 1
