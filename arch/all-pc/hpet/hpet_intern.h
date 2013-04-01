#include <exec/libraries.h>
#include <exec/semaphores.h>

struct HPETUnit
{
    IPTR	base;
    IPTR	block;
    const char *Owner;
};

struct HPETBase
{
    struct Library          libnode;
    ULONG		    unitCnt;
    struct HPETUnit	   *units;
    struct SignalSemaphore  lock;
};
