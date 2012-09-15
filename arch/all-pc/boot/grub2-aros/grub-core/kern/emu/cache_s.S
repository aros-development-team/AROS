#ifndef GRUB_MACHINE_EMU
#error "This source is only meant for grub-emu platform"
#endif

#if defined(__i386__) || defined(__x86_64__)
/* Nothing is necessary.  */
#elif defined(__sparc__)
#include "../sparc64/cache.S"
#elif defined(__mips__)
/* On MIPS we must go through standard functions.  */
#include <grub/symbol.h>

FUNCTION (grub_cpu_flush_cache)
FUNCTION (grub_arch_sync_caches)
	.set nomacro
	.set noreorder
	lui $t0, %hi(_flush_cache)
	addui $t0, $t0, %lo(_flush_cache)
	move $a3, $zero
	jr $t0
	 nop
	.set reorder
	.set macro
#elif defined(__powerpc__)
#include "../powerpc/cache.S"
#elif defined(__ia64__)
#else
#error "No target cpu type is defined"
#endif
