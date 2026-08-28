#ifndef KERNEL_IPI_H_
#define KERNEL_IPI_H_
/*
    Copyright � 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * Private kernel IPI message types. Values are bit flags so the
 * BCM2836 mailbox SET register (which OR-coalesces concurrent writes
 * from the same sender) can carry multiple message types in one IPI
 * delivery without losing any. The receiver dispatches by testing
 * each bit. Source CPU is encoded in the top 4 bits (see senders).
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

#include <utility/hooks.h>

#define IPI_CALL_HOOK_MAX_ARGS  5

struct IPIHook
{
    struct Hook ih_Hook;
    IPTR        ih_Args[IPI_CALL_HOOK_MAX_ARGS];
};

extern void core_IPIInit(void);
extern int core_DoCallIPI(struct Hook *hook, void *cpu_mask, int async,
                          int nargs, IPTR *args, APTR _KB);

/*
 * Split claim/commit API plus cancellation, so exec can validate the
 * target's lifetime atomically with the enqueue (see rom/exec/signal.c)
 * and flush stale calls at task teardown (see rom/exec/remtask.c).
 * KERNEL_IPI_CALL_CANCELABLE keys exec's lifetime-safe path off this
 * header; platforms without it fall back to plain core_DoCallIPI.
 */
struct CallIPIEntry;

extern struct CallIPIEntry *core_ClaimCallIPI(int cpu);
extern void core_CommitCallIPI(struct CallIPIEntry *cie, int cpu,
                               struct Hook *hook, int nargs, IPTR *args);
extern void core_AbortCallIPI(struct CallIPIEntry *cie, int cpu);
extern void core_CancelCallIPIs(APTR hookEntry, IPTR matchArg);

#define KERNEL_IPI_CALL_CANCELABLE

#endif
