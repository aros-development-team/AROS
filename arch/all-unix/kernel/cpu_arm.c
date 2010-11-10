#include <signal.h>

#include "kernel_intern.h"

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

struct SignalTranslation sigs[] = {
    {SIGILL   ,  5,  6},
    {SIGTRAP  ,  5,  1},
    {SIGBUS   ,  1, 13},
    {SIGFPE   ,  1, 16},
    {SIGSEGV  ,  1, 14},
#ifdef HOST_OS_linux
    {SIGSTKFLT,  1, 12},
#endif
    {-1       , -1, -1}
};
