#include <aros/debug.h>
#include <asm/amcc440.h>
#include <asm/io.h>
#include "../kernel/syscall.h"
#include "../kernel/kernel_intern.h"

#include <aros/asmcall.h>
#include <hardware/intbits.h>
#include <exec/execbase.h>

#include <inttypes.h>
#include <proto/exec.h>
#include <proto/timer.h>

#include "lowlevel.h"

uint32_t tbc_expected;
uint32_t tbc_achieved;
int32_t corr;

//inline uint32_t __attribute__((const)) tick2usec(uint32_t tick)
//{
//    uint32_t retval;
//    uint64_t tmp = ((uint64_t)tick) * 64424510;
//
//    retval = (tmp + 0x7fffffff) >> 32;
//
//    return retval;
//}
//
//inline uint32_t __attribute__((const)) usec2tick(uint32_t usec)
//{
//    uint32_t retval;
//    uint64_t tmp = ((uint64_t)usec) * 286331150203ULL;
//
//    retval = (tmp + 0x7fffffff) >> 32;
//
//    return retval;
//}

inline uint32_t __attribute__((const)) tick2usec(uint32_t tick)
{
    struct KernelBase *KernelBase = getKernelBase();
    uint32_t retval;
    uint64_t tmp = ((uint64_t)tick) * 1000000;

    retval = (uint32_t)((tmp) / KernelBase->kb_OPBFreq);

    return retval;
}

inline uint32_t __attribute__((const)) usec2tick(uint32_t usec)
{
    uint32_t retval;
    struct KernelBase *KernelBase = getKernelBase();
    uint64_t tmp = ((uint64_t)usec) * KernelBase->kb_OPBFreq;

    retval = (tmp) / 1000000;

    return retval;
}

void EClockUpdate(struct TimerBase *TimerBase)
{
    uint32_t time;
    uint32_t diff;
    int show = 0;
    struct KernelBase *KernelBase = getKernelBase();

    time = inl(GPT0_TBC);
    diff = (time - TimerBase->tb_prev_tick);
    TimerBase->tb_prev_tick = time;

    TimerBase->tb_ticks_total += diff;

    TimerBase->tb_ticks_sec += diff;
    TimerBase->tb_ticks_elapsed += diff;

    if (TimerBase->tb_ticks_sec >= KernelBase->kb_OPBFreq)
    {
        TimerBase->tb_ticks_sec -= KernelBase->kb_OPBFreq;
        TimerBase->tb_CurrentTime.tv_secs++;
        //show = 1;
    }

    if (TimerBase->tb_ticks_elapsed >= KernelBase->kb_OPBFreq)
    {
        TimerBase->tb_ticks_elapsed -= KernelBase->kb_OPBFreq;
        TimerBase->tb_Elapsed.tv_secs++;
    }

    TimerBase->tb_Elapsed.tv_micro = tick2usec(TimerBase->tb_ticks_elapsed);
    TimerBase->tb_CurrentTime.tv_micro = tick2usec(TimerBase->tb_ticks_sec);

//    if (show)
//        bug("[timer] CurrentTime: %d:%06d\n", TimerBase->tb_CurrentTime.tv_secs, TimerBase->tb_CurrentTime.tv_micro);
        //bug("[timer] %08x %08x %d \n", tbc_expected, tbc_achieved, corr);
}

void EClockSet(struct TimerBase *TimerBase)
{
    TimerBase->tb_ticks_sec = usec2tick(TimerBase->tb_CurrentTime.tv_micro);
}

void TimerSetup(struct TimerBase *TimerBase, uint32_t waste)
{
	struct KernelBase *KernelBase = getKernelBase();
    int32_t delay = KernelBase->kb_OPBFreq / 50;  /* 50Hz in worst case */
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
                if (delay > usec2tick(time.tv_micro))
                    delay = usec2tick(time.tv_micro);
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
                if (delay > usec2tick(time.tv_micro))
                    delay = usec2tick(time.tv_micro);
            }
        }
    }

    current_time = inl(GPT0_TBC);
    delay -= current_time - waste + corr;

    if (delay < 100) delay = 100;

    tbc_expected = current_time + delay;

    outl(delay, GPT0_DCT0);
}

/*
    This is a slightly faster version of AddTime() that we use here. It
    also saves the function call overhead...
*/
#define FastAddTime(d, s)\
    (d)->tv_micro += (s)->tv_micro;\
    (d)->tv_secs += (s)->tv_secs;\
    if((d)->tv_micro > 999999) {\
        (d)->tv_secs++;\
        (d)->tv_micro -= 1000000;\
    }

BOOL timer_addToWaitList(struct TimerBase *, struct MinList *, struct timerequest *);

void GPTHandler(struct TimerBase *TimerBase, struct ExecBase *SysBase)
{
    uint32_t startup_time = inl(GPT0_TBC);

    if (inl(GPT0_DCIS) & GPT0_DCIS_DCIS)
    {
        struct timerequest *tr, *next;
        outl(GPT0_DCIS_DCIS, GPT0_DCIS);
        EClockUpdate(TimerBase);

        tbc_achieved = startup_time;

        /* Timer errors bigger than approx 15 microseconds shouldn't be taken into account */
        if (tbc_achieved > tbc_expected && (tbc_achieved - tbc_expected) < 1000)
        {
            corr = ((int32_t)(tbc_achieved - tbc_expected))-1;
        }


        /*
            Go through the "wait for x seconds" list and return requests
            that have completed. A completed request is one whose time
            is less than that of the elapsed time.
        */
        ForeachNodeSafe(&TimerBase->tb_Lists[TL_VBLANK], tr, next)
        {
            if(CmpTime(&TimerBase->tb_Elapsed, &tr->tr_time) <= 0)
            {
                if (tr == &TimerBase->tb_vblank_timerequest)
                {
                    struct IntVector *iv = &SysBase->IntVects[INTB_VERTB];

                    /* VBlank Emu */

                    if (iv->iv_Code)
                    {
                        AROS_UFC5(void, iv->iv_Code,
                            AROS_UFCA(ULONG, 0, D1),
                            AROS_UFCA(ULONG, 0, A0),
                            AROS_UFCA(APTR, iv->iv_Data, A1),
                            AROS_UFCA(APTR, iv->iv_Code, A5),
                            AROS_UFCA(struct ExecBase *, SysBase, A6)
                        );
                    }
                    /* Automatically requeue/reactivate request */

                    Remove((struct Node *)tr);
                    tr->tr_node.io_Command = TR_ADDREQUEST;
                    tr->tr_time.tv_secs = 0;
                    tr->tr_time.tv_micro = 1000000 / SysBase->VBlankFrequency;
                    AddTime(&tr->tr_time, &TimerBase->tb_Elapsed);

                    timer_addToWaitList(TimerBase, &TimerBase->tb_Lists[TL_VBLANK], tr);

                    if (--SysBase->Elapsed == 0)
                    {
                    	SysBase->SysFlags |= 0x2000;
                    	SysBase->AttnResched |= 0x80;
                    }
                }
                else
                {
                    /* This request has finished */
                    Remove((struct Node *)tr);
                    tr->tr_time.tv_secs = tr->tr_time.tv_micro = 0;
                    tr->tr_node.io_Error = 0;
                    ReplyMsg((struct Message *)tr);
                }
            }
            else
            {
                /*
                    The first request hasn't finished, as all requests are in
                    order, we don't bother searching through the remaining
                */
                break;
            }
        }

        /*
            The other this is the "wait until a specified time". Here a request
            is complete if the time we are waiting for is before the current time.
        */
        ForeachNodeSafe(&TimerBase->tb_Lists[TL_WAITVBL], tr, next)
        {
            if(CmpTime(&TimerBase->tb_CurrentTime, &tr->tr_time) <= 0)
            {
                /* This request has finished */
                Remove((struct Node *)tr);
                tr->tr_time.tv_secs = tr->tr_time.tv_micro = 0;
                tr->tr_node.io_Error = 0;
                ReplyMsg((struct Message *)tr);
            }
            else
            {
                /*
                    The first request hasn't finished, as all requests are in
                    order, we don't bother searching through the remaining
                */
                break;
            }
        }
        TimerSetup(TimerBase, startup_time);
    }
}
