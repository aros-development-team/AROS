
#include <grub/symbol.h>

	.set noreorder
	.set nomacro

FUNCTION (grub_cpu_flush_cache)
FUNCTION (grub_arch_sync_caches)
#include "cache_flush.S"
	j $ra

FUNCTION (grub_arch_sync_dma_caches)
	move $t2, $a0
	addu $t3, $a0, $a1
	srl $t2, $t2, 5
	sll $t2, $t2, 5
	addu $t3, $t3, 0x1f
	srl $t3, $t3, 5
	sll $t3, $t3, 5
	move $t0, $t2
	subu $t1, $t3, $t2
1:
	cache 1, 0($t0)
#ifdef GRUB_MACHINE_MIPS_LOONGSON
	cache 1, 1($t0)
	cache 1, 2($t0)
	cache 1, 3($t0)

	addiu $t1, $t1, -0x20
	bne $t1, $zero, 1b
	 addiu $t0, $t0, 0x20
#else
	addiu $t1, $t1, -4
	bne $t1, $zero, 1b
	 addiu $t0, $t0, 0x4
#endif
	sync
	move $t0, $t2
	subu $t1, $t3, $t2
2:
#ifdef GRUB_MACHINE_MIPS_LOONGSON
	cache 0, 0($t0)
	addiu $t1, $t1, -0x20
	bne $t1, $zero, 2b
	 addiu $t0, $t0, 0x20
#else
	cache 0, 0($t0)
	addiu $t1, $t1, -4
	bne $t1, $zero, 2b
	 addiu $t0, $t0, 0x4
#endif
	sync
	move $t0, $t2
	subu $t1, $t3, $t2
2:
#ifdef GRUB_MACHINE_MIPS_LOONGSON
	cache 23, 0($t0)
	addiu $t1, $t1, -0x20
	bne $t1, $zero, 2b
	 addiu $t0, $t0, 0x20
#else
	cache 23, 0($t0)
	addiu $t1, $t1, -0x4
	bne $t1, $zero, 2b
	 addiu $t0, $t0, 0x4
#endif
	sync

	jr $ra