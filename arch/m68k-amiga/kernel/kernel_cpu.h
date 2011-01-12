/*
 * M68K CPU-specific definitions.
 *
 */

#ifndef KERNEL_CPU_H_
#define KERNEL_CPU_H_

#include <proto/exec.h>

#include "cpu_m68k.h"
 
/* User/supervisor mode switching */
#define cpumode_t __unused int

/* Only used on protected memory systems. On the
 * m68k-amiga, these are unneeded.
 */
#define goSuper() (0)
#define goUser()  do { } while (0)
#define goBack(mode)  do { } while (0)


/* Syscalls use the F-line emulation trap,
 * via instruction 'F405' (AROS), with
 * %a0 set to 0x41524f53 ("AROS")
 * %d0 is the syscall function to call
 * %d1-%d7,%a1-%a6 are available for
 * arguments. All registers *except* for
 * %d0 are preserved.
 *
 * I would like to use 0xA405, but UAE stole
 * all the A-Line instructions!
 *
 * I used to use 0xF405, but that is a valid
 * instruction on the 68030.
 */
#define KRN_SYSCALL_INST	0xFF05
#define KRN_SYSCALL_MAGIC	0x41524F53
#define krnSysCall(x)	asm volatile ( \
				"move.l %0,%%d0\n" \
				"move.l %2,%%a0\n" \
				".word %c1\n" \
				: : "g" (x), "i" (KRN_SYSCALL_INST), "i" (KRN_SYSCALL_MAGIC) \
				: "%d0", "%a0");

#endif /* _KERNEL_CPU_H */
