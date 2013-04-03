#include "sysmon_intern.h"

#include <devices/timer.h>
#include <clib/alib_protos.h>

/* Timer information */
static struct MsgPort *     timerport = NULL;
static struct timerequest * timermsg = NULL;
static ULONG                SIG_TIMER = 0;

/* Timer functions */
static BOOL InitTimer(struct SysMonData *smdata)
{
    if((timerport = CreatePort(0,0)) == NULL)
        return FALSE;

    if((timermsg = (struct timerequest *) CreateExtIO(timerport, sizeof(struct timerequest))) == NULL)
    {
        DeletePort(timerport);
        timerport = NULL;
        return FALSE;
    }
        
    if(OpenDevice("timer.device", UNIT_VBLANK, ((struct IORequest *) timermsg), 0) != 0)
    {
        DeletePort(timerport);
        timerport = NULL;
        DeleteExtIO((struct IORequest *)timermsg);
        timermsg = NULL;
        return FALSE;
    }

    SIG_TIMER = 1 << timerport->mp_SigBit;

    return TRUE;
}

VOID SignalMeAfter(ULONG msecs)
{
    timermsg->tr_node.io_Command = TR_ADDREQUEST;
    timermsg->tr_time.tv_secs = msecs / 1000;
    timermsg->tr_time.tv_micro = (msecs % 1000) * 1000;
    SendIO((struct IORequest *)timermsg);
}

static VOID DeInitTimer(struct SysMonData *smdata)
{
    if (timermsg != NULL)
    {
	    AbortIO((struct IORequest *)timermsg);
	    WaitIO((struct IORequest *)timermsg);
	    CloseDevice((struct IORequest *)timermsg);
	    DeleteExtIO((struct IORequest *)timermsg);
    }

	if(timerport != NULL) DeletePort(timerport);
}

ULONG GetSIG_TIMER()
{
    return SIG_TIMER;
}

struct SysMonModule timermodule =
{
    .Init = InitTimer,
    .DeInit = DeInitTimer,
};

