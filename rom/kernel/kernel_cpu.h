/*
 * CPU-specific stuff.
 */

struct AROSCPUContext
{
    ULONG empty;
};

#define cpumode_t __unused char
#define goSuper() 0
#define goUser()
#define goBack(mode)
