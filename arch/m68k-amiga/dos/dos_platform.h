#ifndef DOS_PLATFORM_H
#define DOS_PLATFORM_H

/* Amiga interrupts and exceptions use the supervisor stack.  Keep a
 * conservative default for arbitrary applications while allowing measured
 * system processes to request the classic 4 KiB process stack. */
#define PROC_STACKSIZE     8192
#define PROC_MINSTACKSIZE  4096

#endif
