/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <asm/mpc5200b.h>
#include <asm/io.h>
#include <aros/asmcall.h>
#include <exec/execbase.h>

#include <inttypes.h>
#include <proto/exec.h>
#include <proto/timer.h>

#include "lowlevel.h"

// tick2usec -  64424510 for 66.666666MHz
// tick2usec - 130150524 for 33.000000MHz

inline uint32_t __attribute__((const)) tick2usec(uint32_t tick)
{
    uint32_t retval;
    uint64_t tmp = ((uint64_t)tick) * 130150524;

    retval = (tmp + 0x7fffffff) >> 32;

    return retval;
}

inline uint32_t __attribute__((const)) usec2tick(uint32_t usec)
{
    uint32_t retval;
//    uint64_t tmp = ((uint64_t)usec) * 286331150203ULL;
//
//    retval = (tmp + 0x7fffffff) >> 32;

    retval = usec * 33;

    return retval;
}

void EClockUpdate(struct TimerBase *TimerBase)
{
    uint32_t time;
    uint32_t diff;
    int show = 0;

    time = mftbl();
    diff = (time - TimerBase->tb_prev_tick);
    TimerBase->tb_prev_tick = time;

    TimerBase->tb_ticks_total += diff;

    TimerBase->tb_ticks_sec += diff;
    TimerBase->tb_ticks_elapsed += diff;

    if (TimerBase->tb_ticks_sec >= 33000000)
    {
        TimerBase->tb_ticks_sec -= 33000000;
        TimerBase->tb_CurrentTime.tv_secs++;

        show = 1;
    }

    if (TimerBase->tb_ticks_elapsed >= 33000000)
    {
        TimerBase->tb_ticks_elapsed -= 33000000;
        TimerBase->tb_Elapsed.tv_secs++;
    }

    TimerBase->tb_Elapsed.tv_micro = tick2usec(TimerBase->tb_ticks_elapsed);
    TimerBase->tb_CurrentTime.tv_micro = tick2usec(TimerBase->tb_ticks_sec);

//    if (show)
//    {
//        bug("[timer] CurrentTime: %d:%06d\n", TimerBase->tb_CurrentTime.tv_secs, TimerBase->tb_CurrentTime.tv_micro);
//        bug("[timer] %08x %08x %d %d \n", tbc_expected, tbc_achieved, tbc_achieved-tbc_expected, corr);
//    }
}

void EClockSet(struct TimerBase *TimerBase)
{
    TimerBase->tb_ticks_sec = usec2tick(TimerBase->tb_CurrentTime.tv_micro);
}

extern volatile slt_t *slice_timer;

void TimerSetup(struct TimerBase *TimerBase, uint32_t waste)
{
    int32_t delay = 660000;  /* 50Hz in worst case */
    struct timeval time;
    struct timerequest *tr;
    uint32_t current_time;

    tr = (struct timerequest *)GetHead(&TimerBase->tb_Lists[TL_WAITVBL]);

    if (tr)
    {
        time.tv_micro = tr->tr_time.tv_micro;
        time.tv_secs  = tr->tr_time.tv_secs;

        SubTime(&time, &TimerBase->tb_CurrentTime);

        if ((LONG)time.tv_secs < 0)
        {
            delay = 0;
        }
        else if (time.tv_secs == 0)
        {
//            if (time.tv_micro < 20000)
//            {
                if (delay > usec2tick(time.tv_micro))
                    delay = usec2tick(time.tv_micro);
//            }
        }
    }

    tr = (struct timerequest *)GetHead(&TimerBase->tb_Lists[TL_VBLANK]);

    if (tr)
    {
        time.tv_micro = tr->tr_time.tv_micro;
        time.tv_secs  = tr->tr_time.tv_secs;

        SubTime(&time, &TimerBase->tb_Elapsed);

        if ((LONG)time.tv_secs < 0)
        {
            delay = 0;
        }
        else if (time.tv_secs == 0)
        {
//            if (time.tv_micro < 20000)
//            {
                if (delay > usec2tick(time.tv_micro))
                    delay = usec2tick(time.tv_micro);
//            }
        }
    }

    current_time = mftbl();
    delay -= ((int32_t)(current_time - waste)) + corr;

    if (delay < 256) delay = 256;

    tbc_expected = current_time + delay;

    /* Stop the timer */
    outl(SLT_CF_INTRENA, &slice_timer->slt_cf);
    /* Set the delay */
    outl(delay, &slice_timer->slt_tc);
    /* Let timer go */
    outl(SLT_CF_INTRENA | SLT_CF_ENABLE, &slice_timer->slt_cf);
    /* Clear interrupt request. */
    outl(SLT_TS_ST, &slice_timer->slt_ts);
}
