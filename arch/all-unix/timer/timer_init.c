/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

/*
 * UNIX-hosted timer driver.
 * Unix operating systems have only one usable timer, producing SIGALRM.
 * In this implementation we run the timer with a fixed frequency, which
 * is a multiple of VBlank frequency.
 * TODO: Rewrite UNIX-hosted timer.device to use variable delays, for improved accuracy.
 */

#include <aros/bootloader.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <proto/arossupport.h>
#include <proto/bootloader.h>
#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/kernel.h>

#include "timer_intern.h"
#include "timer_macros.h"

#define timeval sys_timeval

#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#undef timeval

/* Android is not a true Linux ;-) */
#ifdef HOST_OS_android
#undef HOST_OS_linux
#endif

#ifdef HOST_OS_linux
#define LIBC_NAME "libc.so.6"
#endif

#ifdef HOST_OS_darwin
#define LIBC_NAME "libSystem.dylib"
#endif

#ifndef LIBC_NAME
#define LIBC_NAME "libc.so"
#endif

/*
 * In Linux stdlib.h #define's atoi() to something internal, causing link failure
 * because we link our kickstart binaries against AROS libraries, and not against
 * host OS ones.
 * Explicit prototype here avoids this.
 */
int atoi(const char *nptr);

/* The host's monotonic clock, in microseconds. */
static UQUAD HostNow(struct TimerBase *TimerBase)
{
    struct timespec ts;

    TimerBase->tb_Platform.clock_gettime(CLOCK_MONOTONIC, &ts);
    AROS_HOST_BARRIER
    return (UQUAD)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

/* Handle periodic timer and drive exec VBlank */
static void TimerTick(struct TimerBase *TimerBase, struct ExecBase *SysBase)
{
    /*
     * Advance the clocks by the time that went by. A fixed period per
     * SIGALRM lost time for good: the host does not queue a signal that is
     * already pending, so ticks that came while AROS was not running (the
     * host did not schedule it, or interrupts were off) were one, and the
     * uptime and system time fell behind - by over a second at a time.
     * The host's monotonic clock counts all of it. Without clock_gettime()
     * a tick is still the period.
     */
    if (TimerBase->tb_Platform.clock_gettime)
    {
        UQUAD now = HostNow(TimerBase);
        UQUAD d = now - TimerBase->tb_Platform.tb_HostLast;
        struct timeval delta;

        TimerBase->tb_Platform.tb_HostLast = now;
        delta.tv_secs  = d / 1000000;
        delta.tv_micro = d % 1000000;
        ADDTIME(&TimerBase->tb_CurrentTime, &delta);
        ADDTIME(&TimerBase->tb_Elapsed, &delta);
        TimerBase->tb_Platform.tb_HostElapsed += d;
        TimerBase->tb_ticks_total = TimerBase->tb_Platform.tb_HostElapsed * TimerBase->tb_eclock_rate / 1000000;
    }
    else
    {
        ADDTIME(&TimerBase->tb_CurrentTime, &TimerBase->tb_Platform.tb_VBlankTime);
        ADDTIME(&TimerBase->tb_Elapsed, &TimerBase->tb_Platform.tb_VBlankTime);
        TimerBase->tb_ticks_total++;
    }

    handleMicroHZ(TimerBase, SysBase);

    /* Now handle VBlank emulation via divisor */
    TimerBase->tb_Platform.tb_TimerCount++;
    if (TimerBase->tb_Platform.tb_TimerCount == TimerBase->tb_Platform.tb_VBlankTicks)
    {
        vblank_Cause(SysBase);
        handleVBlank(TimerBase, SysBase);

        TimerBase->tb_Platform.tb_TimerCount = 0;
    }
}

#define KernelBase TimerBase->tb_KernelBase

static int Timer_Init(struct TimerBase *TimerBase)
{
    APTR BootLoaderBase;
    struct itimerval interval;
    int ret;

    HostLibBase = OpenResource("hostlib.resource");
    if (!HostLibBase)
        return FALSE;

    TimerBase->tb_Platform.libcHandle = HostLib_Open(LIBC_NAME, NULL);
    if (!TimerBase->tb_Platform.libcHandle)
        return FALSE;

    TimerBase->tb_Platform.setitimer = HostLib_GetPointer(TimerBase->tb_Platform.libcHandle, "setitimer", NULL);
    if (!TimerBase->tb_Platform.setitimer)
        return FALSE;
    /* optional: without it a tick counts as its period */
    TimerBase->tb_Platform.clock_gettime = HostLib_GetPointer(TimerBase->tb_Platform.libcHandle, "clock_gettime", NULL);

    /* Install timer IRQ handler */
    TimerBase->tb_TimerIRQHandle = KrnAddIRQHandler(SIGALRM, TimerTick, TimerBase, SysBase);
    if (!TimerBase->tb_TimerIRQHandle)
        return FALSE;

    /* Our defaults: 50 Hz VBlank and 4x timer rate. 1x gives very poor results. */
    SysBase->VBlankFrequency = 50;
    TimerBase->tb_Platform.tb_VBlankTicks = 4;

    /*
     * Since we are software-driven, we can just ask the user which
     * frequencies he wishes to use.
     */
    BootLoaderBase = OpenResource("bootloader.resource");
    if (BootLoaderBase)
    {
        struct List *args = GetBootInfo(BL_Args);

        if (args)
        {
            struct Node *node;

            for (node = args->lh_Head; node->ln_Succ; node = node->ln_Succ)
            {
                if (strncasecmp(node->ln_Name, "vblank=", 7) == 0)
                    SysBase->VBlankFrequency = atoi(&node->ln_Name[7]);
                else if (strncasecmp(node->ln_Name, "tickrate=", 9) == 0)
                    TimerBase->tb_Platform.tb_VBlankTicks = atoi(&node->ln_Name[9]);
            }
        }
    }

    /* Calculate effective EClock timer frequency. Set it also in ExecBase public field. */
    TimerBase->tb_eclock_rate = SysBase->VBlankFrequency * TimerBase->tb_Platform.tb_VBlankTicks;
    SysBase->ex_EClockFrequency = TimerBase->tb_eclock_rate;
    D(bug("[Timer_Init] Timer frequency is %d\n", TimerBase->tb_eclock_rate));

    /* Calculate timer period in us */
    TimerBase->tb_Platform.tb_VBlankTime.tv_secs  = 0;
    TimerBase->tb_Platform.tb_VBlankTime.tv_micro = 1000000 / TimerBase->tb_eclock_rate;

    /* Start up our system timer. */
    if (TimerBase->tb_Platform.clock_gettime)
        TimerBase->tb_Platform.tb_HostLast = HostNow(TimerBase);
    interval.it_interval.tv_sec  = interval.it_value.tv_sec  = 0;
    interval.it_interval.tv_usec = interval.it_value.tv_usec = TimerBase->tb_Platform.tb_VBlankTime.tv_micro;

    HostLib_Lock();

    ret = TimerBase->tb_Platform.setitimer(ITIMER_REAL, &interval, NULL);
    AROS_HOST_BARRIER

    HostLib_Unlock();

    D(bug("[Timer_Init] setitimer() returned %d\n", ret));
    return !ret;
}

static int Timer_Expunge(struct TimerBase *TimerBase)
{
    if (!HostLibBase)
        return TRUE;

    if (TimerBase->tb_TimerIRQHandle)
        KrnRemIRQHandler(TimerBase->tb_TimerIRQHandle);

    if (TimerBase->tb_Platform.libcHandle)
        HostLib_Close(TimerBase->tb_Platform.libcHandle, NULL);

    return TRUE;
}

ADD2INITLIB(Timer_Init, 0)
ADD2EXPUNGELIB(Timer_Expunge, 0)
