#include <signal.h>

struct SignalTranslation sigs[] = {
    {SIGILL   ,  4,  6},
    {SIGTRAP  ,  9,  1},
    {SIGBUS   ,  2, 13},
    {SIGFPE   , 11, 16},
    {SIGSEGV  ,  2, 14},
    {SIGSTKFLT, 14, 12},
    {-1       , -1, -1}
};
