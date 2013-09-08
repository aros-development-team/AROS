/*
    Copyright 2009-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>

/* 
    The purpose of this file is to provide implementation for C functions part
    of arosnixc.library in code where one does not want to use this library.
*/

struct timezone;

int gettimeofday (struct timeval * tv,struct timezone * tz)
{
    struct MsgPort * timerport = CreateMsgPort();
    struct timerequest * timereq = (struct timerequest *)CreateIORequest(timerport, sizeof(*timereq));


    if (timereq)
    {
        if (OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)timereq, 0) == 0)
        {
            #define TimerBase ((struct Device *)timereq->tr_node.io_Device)

            GetSysTime(tv);
            
            #undef TimerBase
            
            CloseDevice((struct IORequest *)timereq);
        }
    }
    
    DeleteIORequest((struct IORequest *)timereq);
    DeleteMsgPort(timerport);

    return 0;
}
