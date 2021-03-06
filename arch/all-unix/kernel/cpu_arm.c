/*
    Copyright (C) 1995-2021, The AROS Development Team. All rights reserved.
*/

#include "kernel_base.h"
#include "kernel_intern.h"

#include <signal.h>

/*
 * ARM exceptions are the following:
 * 0 - Reset
 * 1 - Data abort
 * 2 - FIQ
 * 3 - IRQ
 * 4 - Prefetch abort
 * 5 - Invalid instruction
 * 6 - Software interrupt (not simulated)
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
