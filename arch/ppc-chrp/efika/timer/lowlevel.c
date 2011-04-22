#define DEBUG 1

#include <aros/debug.h>
#include <asm/mpc5200b.h>
#include <asm/io.h>

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

volatile slt_t *slice_timer;

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

void SliceHandler(struct TimerBase *TimerBase, struct ExecBase *SysBase)
{
    uint32_t startup_time = mftbl();

    if (inl(&slice_timer->slt_ts) & SLT_TS_ST)
    {
        struct timerequest *tr, *next;

        /* Clear interrupt request. */
        outl(SLT_TS_ST, &slice_timer->slt_ts);
        sync();
        /* Stop the timer */
        outl(0, &slice_timer->slt_cf);
//        outl(SLT_CF_RUNWAIT | SLT_CF_INTRENA | SLT_CF_ENABLE, &slice_timer->slt_cf);

//        D(bug("[timer] SliceHandler on %08x%08x slt->cv=%08x\n", (uint32_t)(mftb() >> 32),mftbl(),inl(&slice_timer->slt_cv)));

        EClockUpdate(TimerBase);

        tbc_achieved = startup_time;

        /* Timer errors bigger than approx 15 microseconds shouldn't be taken into account */
        //if (tbc_achieved > tbc_expected) // && (tbc_achieved - tbc_expected) < 1000)
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
