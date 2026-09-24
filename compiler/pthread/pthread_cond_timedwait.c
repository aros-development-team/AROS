/*
  Copyright (C) 2014 Szilard Biro
  Copyright (C) 2018 Harry Sintonen
  Copyright (C) 2019 Stefan "Bebbo" Franke - AmigaOS 3 port

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
    ULONG sigs = SIGBREAKF_CTRL_C;
    struct MsgPort timermp;
    struct timerequest timerio;
    struct Task *task;

    DB2(bug("%s(%p, %p, %p, %d)\n", __FUNCTION__, cond, mutex, abstime, relative));

    if (cond == NULL || mutex == NULL)
        return EINVAL;

    // initialize static conditions
    if (SemaphoreIsInvalid(&cond->semaphore))
        pthread_cond_init(cond, NULL);

    task = GET_THIS_TASK;

    if (abstime)
    {
        struct timeval tvrel;
        struct timespec rel, now;
        clockid_t clk;

        // A garbage nanosecond field is invalid usage (POSIX: EINVAL),
        // on both the absolute and the relative entry points.
        if (abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000)
            return EINVAL;

        // open timer.device
        if (!OpenTimerDevice((struct IORequest *)&timerio, &timermp, task))
        {
            CloseTimerDevice((struct IORequest *)&timerio);
            return EINVAL;
        }

        if (!relative)
        {
            // Absolute deadlines are read against the clock the condvar
            // was initialised with (CLOCK_REALTIME unless the attr asked
            // for CLOCK_MONOTONIC, e.g. Mesa's u_cnd_monotonic_*). Reading
            // a monotonic deadline against the realtime clock - as this
            // code used to do unconditionally via gettimeofday() - puts the
            // interval decades off and the wait never ends.
            clk = cond->cond_clock;
            if (clock_gettime(clk, &now) != 0)
            {
                CloseTimerDevice((struct IORequest *)&timerio);
                return EINVAL;
            }
            // Signed 64-bit subtraction in the timespec domain: a deadline
            // already past yields a negative rel, never a wrapped timeval.
            rel.tv_sec = abstime->tv_sec - now.tv_sec;
            rel.tv_nsec = abstime->tv_nsec - now.tv_nsec;
            if (rel.tv_nsec < 0)
            {
                rel.tv_sec--;
                rel.tv_nsec += 1000000000L;
            }
        }
        else
        {
            rel = *abstime;
        }

        // Clamp non-positive intervals to a minimal wait instead of
        // returning ETIMEDOUT up front or issuing a zero-length
        // timer.device request (which would wait forever): the waiter is
        // still enqueued below, so a concurrent signal wins and a timeout
        // of "now" reports the condition if it is already signalled, per
        // POSIX, while an unsignalled one still times out promptly.
        if (rel.tv_sec < 0 || (rel.tv_sec == 0 && rel.tv_nsec <= 0))
        {
            rel.tv_sec = 0;
            rel.tv_nsec = 1000000L; // 1 ms
        }
        tvrel.tv_sec = rel.tv_sec;
        tvrel.tv_usec = rel.tv_nsec / 1000;
        if (!timerisset(&tvrel))
        {
            tvrel.tv_sec = 0;
            tvrel.tv_usec = 1; // never a zero-length request
        }

        // prepare the device command and send it
        timerio.tr_node.io_Command = TR_ADDREQUEST;
        timerio.tr_node.io_Flags = 0;
        timerio.tr_time.tv_secs = tvrel.tv_sec;
        timerio.tr_time.tv_micro = tvrel.tv_usec;
        sigs |= (1 << timermp.mp_SigBit);
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
    waiter.sigbit = signal;
    sigs |= 1 << waiter.sigbit;

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

    if (waiter.sigbit != SIGB_COND_FALLBACK)
        FreeSignal(waiter.sigbit);

    if (abstime)
    {
        // clean up the timerequest
        CloseTimerDevice((struct IORequest *)&timerio);

        // did we timeout?
        if (sigs & (1 << timermp.mp_SigBit))
            return ETIMEDOUT;
        else if (sigs & SIGBREAKF_CTRL_C)
            pthread_testcancel();
    }
    else
    {
        if (sigs & SIGBREAKF_CTRL_C)
            pthread_testcancel();
    }

    return 0;
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
    D(bug("%s(%p, %p, %p)\n", __FUNCTION__, cond, mutex, abstime));

    return _pthread_cond_timedwait(cond, mutex, abstime, FALSE);
}
