#include <aros/symbolsets.h>
#include <proto/exec.h>

#include "battclock_intern.h"

/* auto init */
static int BattClock_Init(struct BattClockBase *BattClockBase)
{
    InitSemaphore(&BattClockBase->sem);

    return 1;
}

ADD2INITLIB(BattClock_Init, 0)
