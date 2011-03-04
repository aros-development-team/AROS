/* startup.S - Startup code for the MIPS.  */
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
#include <grub/offsets.h>
#include <grub/machine/memory.h>
#include <grub/offsets.h>

#define BASE_ADDR 8

.extern __bss_start
.extern _end

	.globl __start, _start, start
	.set noreorder
	.set nomacro
__start:
_start:
start:		

	bal codestart
	 nop
base:	
	. = _start + GRUB_KERNEL_MACHINE_COMPRESSED_SIZE
compressed_size:
	.long 0
	. = _start + GRUB_KERNEL_MACHINE_UNCOMPRESSED_SIZE
uncompressed_size:
	.long 0
codestart:
	/* Save our base.  */
	move $s0, $ra

	/* Parse arguments. Has to be done before relocation.
	So need to do it in asm.  */
#ifdef GRUB_MACHINE_MIPS_YEELOONG
	move $s2, $zero
	move $s3, $zero
	move $s4, $zero
	move $s5, $zero

	/* $a2 has the environment.  */
	addiu $t0, $a2, 1
	beq $t0, $zero, argdone
	 nop
	move $t0, $a2	
argcont:	
	lw $t1, 0($t0)
	beq $t1, $zero, argdone
	 nop
#define DO_PARSE(str, reg) \
	addiu $t2, $s0, (str-base);\
	bal parsestr;\
	 nop	    ;\
	beq $v0, $zero, 1f;\
	 nop		  ;\
	b 2f;\
	 move reg, $v0;	 \
1:	
	DO_PARSE (busclockstr, $s2)
	DO_PARSE (cpuclockstr, $s3)
	DO_PARSE (memsizestr, $s4)
	DO_PARSE (highmemsizestr, $s5)
2:	
	b argcont
	 addiu $t0, $t0, 4	 
parsestr:
	move $v0, $zero
	move $t3, $t1
3:	
	lb $t4, 0($t2)
	lb $t5, 0($t3)
	addiu $t2, $t2, 1
	addiu $t3, $t3, 1
	beq $t5, $zero, 1f
	 nop
	beq $t5, $t4, 3b
	 nop
	bne $t4, $zero, 1f
	 nop

	addiu $t3, $t3, 0xffff
digcont:	
	lb $t5, 0($t3)
	/* Substract '0' from digit.  */
	addiu $t5, $t5, 0xffd0
	bltz $t5, 1f
	 nop
	addiu $t4, $t5, 0xfff7
	bgtz $t4, 1f
	 nop
	/* Multiply $v0 by 10 with bitshifts. */
	sll $v0, $v0, 1
	sll $t4, $v0, 2
	addu $v0, $v0, $t4
	addu $v0, $v0, $t5
	addiu $t3, $t3, 1
	b digcont
	 nop
1:
	jr $ra
	 nop
busclockstr:	.asciiz "busclock="
cpuclockstr:	.asciiz "cpuclock="
memsizestr:	.asciiz "memsize="
highmemsizestr:	.asciiz "highmemsize="
	.p2align 2
argdone:
#endif
	/* Copy the decompressor. */
	lui $t1, %hi(base)
	addiu $t1, $t1, %lo(base)
	lui $t3, %hi(__bss_start)
	addiu $t3, $t3, %lo(__bss_start)
	move $t2, $s0

1:
	beq $t1, $t3, 2f
	 lb $t4, 0($t2)
	sb $t4, 0($t1)
	addiu $t1, $t1, 1
	b 1b
	 addiu $t2, $t2, 1
2:
	/* Clean out its BSS.  */
	lui $t1, %hi(__bss_start)
	addiu $t1, $t1, %lo(__bss_start)
	lui $t2, %hi(_end)
	addiu $t2, $t2, %lo(_end)
1:
	beq $t1, $t2, 2f
	 nop
	sb $zero, 0($t1)
	b 1b
	 addiu $t1, $t1, 1
2:

	/* Decompress the payload. */
	lui $a0, %hi(__bss_start)
	addiu $a0, $a0, %lo(__bss_start)
	lui $t0, %hi(base)
	addiu $t0, $t0, %lo(base)
	subu $a0, $a0, $t0
	addu $a0, $a0, $s0

	lui $a1, %hi(GRUB_MACHINE_LINK_ADDR)
	addiu $a1, %lo(GRUB_MACHINE_LINK_ADDR)
	lw $a2, (GRUB_KERNEL_MACHINE_COMPRESSED_SIZE - BASE_ADDR)($s0)
	lw $a3, (GRUB_KERNEL_MACHINE_UNCOMPRESSED_SIZE - BASE_ADDR)($s0)
	move $s1, $a1

	/* $a0 contains source compressed address, $a1 is destination,
	   $a2 is compressed size, $a3 is uncompressed size. 
	*/
	move $s6, $a3

	lui $sp, %hi(_start - 256)

	bal EXT_C(grub_decompress_core)
	 addiu $sp, $sp, %lo(_start - 256)
	
	move $a0, $s1
	move $a1, $s6

#include "../../kern/mips/cache_flush.S"

	lui $t1, %hi(GRUB_MACHINE_LINK_ADDR)
	addiu $t1, %lo(GRUB_MACHINE_LINK_ADDR)

	jr $t1
	 nop
