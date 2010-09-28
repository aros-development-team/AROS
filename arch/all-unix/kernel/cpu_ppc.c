#include <signal.h>

struct SignalTranslation sigs[] = {
    {SIGILL   ,  4,  7},
    {SIGTRAP  ,  9, 13},
    {SIGBUS   ,  2,  3},
    {SIGFPE   , 11,  7},
    {SIGSEGV  ,  2,  3},
    {SIGSTKFLT, 14,  3},
    {-1       , -1, -1}
};
