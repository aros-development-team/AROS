#include <exec/semaphores.h>

struct HPETUnit
{
    IPTR	base;
    IPTR	block;
    const char *Owner;
};

struct HPETBase
{
    struct Node		    node;
    ULONG		    unitCnt;
    struct HPETUnit	   *units;
    struct SignalSemaphore  lock;
};
