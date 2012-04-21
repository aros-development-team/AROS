#include <aros/debug.h>
#include <asm/amcc440.h>
#include <asm/io.h>

#include <aros/asmcall.h>
#include <hardware/intbits.h>
#include <exec/execbase.h>

#include <inttypes.h>
#include <proto/exec.h>
#include <proto/timer.h>

#include <timer_intern.h>
#include <kernel_intern.h>

inline uint32_t __attribute__((const)) tick2usec(struct TimerBase *TimerBase, uint32_t tick)
{
    uint32_t retval;
    uint64_t tmp = ((uint64_t)tick) * 1000000;

    retval = (uint32_t)((tmp) / KernelBase->kb_PlatformData->pd_OPBFreq);

    return retval;
}

inline uint32_t __attribute__((const)) usec2tick(struct TimerBase *TimerBase, uint32_t usec)
{
    uint32_t retval;
    uint64_t tmp = ((uint64_t)usec) * KernelBase->kb_PlatformData->pd_OPBFreq;

    retval = (tmp) / 1000000;

    return retval;
}

void EClockUpdate(struct TimerBase *TimerBase)
{
    uint32_t time;
    uint32_t diff;

    time = inl(GPT0_TBC);
    diff = (time - TimerBase->tb_prev_tick);
    TimerBase->tb_prev_tick = time;

    TimerBase->tb_ticks_total += diff;

    TimerBase->tb_ticks_sec += diff;
    TimerBase->tb_ticks_elapsed += diff;

    if (TimerBase->tb_ticks_sec >= KernelBase->kb_PlatformData->pd_OPBFreq)
    {
        TimerBase->tb_ticks_sec -= KernelBase->kb_PlatformData->pd_OPBFreq;
        TimerBase->tb_CurrentTime.tv_secs++;
        //show = 1;
    }

    if (TimerBase->tb_ticks_elapsed >= KernelBase->kb_PlatformData->pd_OPBFreq)
    {
        TimerBase->tb_ticks_elapsed -= KernelBase->kb_PlatformData->pd_OPBFreq;
        TimerBase->tb_Elapsed.tv_secs++;
    }

    TimerBase->tb_Elapsed.tv_micro = tick2usec(TimerBase, TimerBase->tb_ticks_elapsed);
    TimerBase->tb_CurrentTime.tv_micro = tick2usec(TimerBase, TimerBase->tb_ticks_sec);

//    if (show)
//        bug("[timer] CurrentTime: %d:%06d\n", TimerBase->tb_CurrentTime.tv_secs, TimerBase->tb_CurrentTime.tv_micro);
        //bug("[timer] %08x %08x %d \n", tbc_expected, tbc_achieved, corr);
}

void EClockSet(struct TimerBase *TimerBase)
{
    TimerBase->tb_ticks_sec = usec2tick(TimerBase, TimerBase->tb_CurrentTime.tv_micro);
}


void TimerSetup(struct TimerBase *TimerBase, uint32_t waste)
{
    int32_t delay = KernelBase->kb_PlatformData->pd_OPBFreq / 50;  /* 50Hz in worst case */
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
            if (time.tv_micro < 20000)
            {
                if (delay > usec2tick(TimerBase, time.tv_micro))
                    delay = usec2tick(TimerBase, time.tv_micro);
            }
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
            if (time.tv_micro < 20000)
            {
                if (delay > usec2tick(TimerBase, time.tv_micro))
                    delay = usec2tick(TimerBase, time.tv_micro);
            }
        }
    }

    current_time = inl(GPT0_TBC);
    delay -= current_time - waste + TimerBase->tb_Platform.corr;

    if (delay < 100) delay = 100;

    TimerBase->tb_Platform.tbc_expected = current_time + delay;

    outl(delay, GPT0_DCT0);
}
