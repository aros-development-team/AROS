#ifndef DOS_PLATFORM_H
#define DOS_PLATFORM_H

/* Amiga interrupts and exceptions use the supervisor stack.  Keep a
 * conservative default for arbitrary applications.  A 4 KiB process stack
 * is not sufficient for all legacy CLI paths through AROS DOS/Exec. */
#define PROC_STACKSIZE     8192
#define PROC_MINSTACKSIZE  PROC_STACKSIZE

#endif
