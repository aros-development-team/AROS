/*
 * CPU-specific stuff. Also needs to be replaced for every architecture.
 */

struct AROSCPUContext
{
    ULONG empty;
};

#define cpumode_t __unused char

#define goSuper() 0
#define goUser()
#define goBack(mode)
