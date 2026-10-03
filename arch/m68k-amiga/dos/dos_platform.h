#ifndef DOS_PLATFORM_H
#define DOS_PLATFORM_H

/* Amiga interrupts and exceptions use the supervisor stack.  Keep a
 * conservative default for arbitrary applications.  A 4 KiB process stack
 * is not sufficient for all legacy CLI paths through AROS DOS/Exec. */
#define PROC_STACKSIZE     8192
#define PROC_MINSTACKSIZE  PROC_STACKSIZE

/* Save Chip RAM on unexpanded machines. Keep the normal packet size when
 * Fast RAM can hold a 4 KiB buffer; the smaller buffer increases the number
 * of filesystem packets needed for buffered I/O.
 */
#define IOBUFSIZE          ((AvailMem(MEMF_FAST | MEMF_LARGEST) >= 4096) ? 4096 : 1024)

/* Only this platform reuses the boot process for a requester-free boot. */
#define DOS_REUSE_BOOT_PROCESS

#endif
