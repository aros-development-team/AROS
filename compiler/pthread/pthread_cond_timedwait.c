/*
  Copyright (C) 2014 Szilard Biro

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <proto/timer.h>
#include <string.h>

#include "pthread_intern.h"
#include "debug.h"

int _pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime, BOOL relative)
{
    CondWaiter waiter;
    BYTE signal;
    ULONG sigs = 0;
    ULONG timermask = 0;
    struct MsgPort timermp;
    struct timerequest timerio;
    struct Task *task;

    DB2(bug("%s(%p, %p, %p)\n", __FUNCTION__, cond, mutex, abstime));

    if (cond == NULL || mutex == NULL)
        return EINVAL;

    pthread_testcancel();

    // initialize static conditions
    if (SemaphoreIsInvalid(&cond->semaphore))
        pthread_cond_init(cond, NULL);

    task = FindTask(NULL);

    if (abstime)
    {
        struct timeval tvabstime;
        // prepare MsgPort
        memset( &timermp, 0, sizeof( timermp ) );
        timermp.mp_Node.ln_Type = NT_MSGPORT;
        timermp.mp_Flags = PA_SIGNAL;
        timermp.mp_SigTask = task;
        signal = AllocSignal(-1);
        if (signal == -1)
        {
            signal = SIGB_TIMER_FALLBACK;
            SetSignal(SIGF_TIMER_FALLBACK, 0);
        }
        timermp.mp_SigBit = signal;
        NEWLIST(&timermp.mp_MsgList);

        // prepare IORequest
        timerio.tr_node.io_Message.mn_Node.ln_Type = NT_MESSAGE;
        timerio.tr_node.io_Message.mn_Node.ln_Pri = 0;
        timerio.tr_node.io_Message.mn_Node.ln_Name = NULL;
        timerio.tr_node.io_Message.mn_ReplyPort = &timermp;
        timerio.tr_node.io_Message.mn_Length = sizeof(struct timerequest);

        // open timer.device
        if (OpenDevice((STRPTR)TIMERNAME, UNIT_MICROHZ, &timerio.tr_node, 0) != 0)
        {
            if (timermp.mp_SigBit != SIGB_TIMER_FALLBACK)
                FreeSignal(timermp.mp_SigBit);

            return EINVAL;
        }

        // prepare the device command and send it
        timerio.tr_node.io_Command = TR_ADDREQUEST;
        timerio.tr_node.io_Flags = 0;
        TIMESPEC_TO_TIMEVAL(&tvabstime, abstime);
        if (!relative)
        {
            struct timeval starttime;
            // absolute time has to be converted to relative
            // GetSysTime can't be used due to the timezone offset in abstime
            gettimeofday(&starttime, NULL);
            timersub(&tvabstime, &starttime, &tvabstime);
        }
        timerio.tr_time.tv_secs = tvabstime.tv_sec;
        timerio.tr_time.tv_micro = tvabstime.tv_usec;
        timermask = 1 << timermp.mp_SigBit;
        sigs |= timermask;
        SendIO((struct IORequest *)&timerio);
    }

    // prepare a waiter node
    waiter.task = task;
    signal = AllocSignal(-1);
    if (signal == -1)
    {
        signal = SIGB_COND_FALLBACK;
        SetSignal(SIGF_COND_FALLBACK, 0);
    }
    waiter.sigmask = 1 << signal;
    sigs |= waiter.sigmask;

    // add it to the end of the list
    ObtainSemaphore(&cond->semaphore);
    AddTail((struct List *)&cond->waiters, (struct Node *)&waiter);
    ReleaseSemaphore(&cond->semaphore);

    // wait for the condition to be signalled or the timeout
    mutex->incond++;
    pthread_mutex_unlock(mutex);
    sigs = Wait(sigs);
    pthread_mutex_lock(mutex);
    mutex->incond--;

    // remove the node from the list
    ObtainSemaphore(&cond->semaphore);
    Remove((struct Node *)&waiter);
    ReleaseSemaphore(&cond->semaphore);

    if (signal != SIGB_COND_FALLBACK)
        FreeSignal(signal);

    if (abstime)
    {
        // clean up the timerequest
        if (!CheckIO((struct IORequest *)&timerio))
        {
            AbortIO((struct IORequest *)&timerio);
            WaitIO((struct IORequest *)&timerio);
        }
        CloseDevice((struct IORequest *)&timerio);

        if (timermp.mp_SigBit != SIGB_TIMER_FALLBACK)
            FreeSignal(timermp.mp_SigBit);

        // did we timeout?
        if (sigs & timermask)
            return ETIMEDOUT;
    }

    return 0;
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
    D(bug("%s(%p, %p, %p)\n", __FUNCTION__, cond, mutex, abstime));

    return _pthread_cond_timedwait(cond, mutex, abstime, FALSE);
}
