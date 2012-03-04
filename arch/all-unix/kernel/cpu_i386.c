#include <signal.h>

#include "kernel_base.h"
#include "kernel_intern.h"

struct SignalTranslation const sigs[] = {
    {SIGILL   ,  4,  6},
    {SIGTRAP  ,  9,  1},
    {SIGBUS   ,  2, 13},
    {SIGFPE   , 11, 16},
    {SIGSEGV  ,  2, 14},
#ifdef HOST_OS_linux
    {SIGSTKFLT, 14, 12},
#endif
    {-1       , -1, -1}
};
