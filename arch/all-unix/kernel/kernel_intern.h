#include <sys/types.h>
#include <signal.h>

struct PlatformData
{
    sigset_t	 sig_int_mask;	/* Mask of signals that Disable() block */
    unsigned int supervisor;
};

struct SignalTranslation
{
    short sig;
    short AmigaTrap;
    short CPUTrap;
};

extern struct SignalTranslation sigs[];
