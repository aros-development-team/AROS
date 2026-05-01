#ifndef KERNEL_IPI_H_
#define KERNEL_IPI_H_
/*
    Copyright � 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * List of all possible (private) kernal IPI message types.
 */

#define IPI_NOP                 0x000
#define IPI_CAUSE               0x001
#define IPI_DISPATCH            0x002
#define IPI_SWITCH              0x003
#define IPI_SCHEDULE            0x004
#define IPI_CLI                 0x005
#define IPI_STI                 0x006
#define IPI_ADDTASK	        0x101
#define IPI_REMTASK	        0x102
#define IPI_REBOOT	        0x10F

#include <utility/hooks.h>

#define IPI_CALL_HOOK_MAX_ARGS  5

struct IPIHook
{
    struct Hook ih_Hook;
    IPTR        ih_Args[IPI_CALL_HOOK_MAX_ARGS];
};

/* Stub: arm-native IPI hook dispatch is not yet implemented. */
static inline int core_DoCallIPI(struct Hook *hook, void *cpu_mask, int async,
                                 int nargs, IPTR *args, APTR _KB)
{
    (void)hook; (void)cpu_mask; (void)async;
    (void)nargs; (void)args; (void)_KB;
    return 0;
}
#endif
