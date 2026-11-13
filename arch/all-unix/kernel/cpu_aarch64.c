/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Host-signal -> AROS/CPU trap translation table for hosted AArch64.
*/

#include "kernel_base.h"
#include "kernel_intern.h"

#include <signal.h>

/*
 * AArch64 uses the same 6-entry exception model as ARM (see the darwin
 * cpu_aarch64.h, EXCEPTIONS_COUNT == 6). The host (Darwin/Linux) delivers CPU
 * faults as POSIX signals; map each to its exec trap (AmigaTrap) and CPU
 * exception (CPUTrap). All of these signals exist on Darwin; SIGSTKFLT is
 * Linux-only and stays guarded.
 */

struct SignalTranslation const sigs[] = {
    {SIGILL   ,  4,  5},
    {SIGTRAP  ,  9,  5},
    {SIGBUS   ,  2,  1},
    {SIGFPE   , 11,  1},
    {SIGSEGV  ,  2,  1},
#ifdef HOST_OS_linux
    {SIGSTKFLT, 14,  1},
#endif
    {-1       , -1, -1}
};
