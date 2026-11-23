#ifndef KERNEL_IPI_H_
#define KERNEL_IPI_H_
/*
    Copyright � 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * Private kernel IPI message types. Bit flags, so the mailbox SET
 * register's OR-coalescing cannot lose one. Source CPU in the top 4 bits.
 */

#define IPI_NOP                 0x000
#define IPI_CAUSE               0x001
#define IPI_DISPATCH            0x002
#define IPI_SWITCH              0x004
#define IPI_SCHEDULE            0x008
#define IPI_CLI                 0x010
#define IPI_STI                 0x020
#define IPI_CALL_HOOK           0x040
#define IPI_ADDTASK             0x080
#define IPI_REMTASK             0x100
#define IPI_REBOOT              0x200

/* struct IPIHook, the pending words and the call queues are shared by all ports */
#include <kernel_ipicall.h>

extern void core_IPIInit(void);
extern int core_DoCallIPI(struct Hook *hook, void *cpu_mask, int async,
                          int nargs, IPTR *args, APTR _KB);

/* Post msg to every core in the mask; a core that has it pending already is not interrupted again */
extern void core_SendIPI(ULONG msg, uint32_t mask);

/*
 * Drop the queued calls for one hook and object (ih_Args[1]) and wait for
 * one that is being run, so exec can free the object afterwards.
 */
extern void core_CancelCallIPIs(APTR hookEntry, IPTR matchArg);

#endif
