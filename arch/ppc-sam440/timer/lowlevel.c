#include <aros/debug.h>
#include <asm/amcc440.h>
#include <asm/io.h>
#include "kernel_syscall.h"
#include "kernel_intern.h"

#include <aros/asmcall.h>
#include <hardware/intbits.h>
#include <exec/execbase.h>

#include <inttypes.h>
#include <proto/exec.h>
#include <proto/timer.h>

#include <timer_intern.h>

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

        TimerBase->tb_Platform.tbc_achieved = startup_time;

        /* Timer errors bigger than approx 15 microseconds shouldn't be taken into account */
        if (TimerBase->tb_Platform.tbc_achieved > TimerBase->tb_Platform.tbc_expected && (TimerBase->tb_Platform.tbc_achieved - TimerBase->tb_Platform.tbc_expected) < 1000)
        {
            TimerBase->tb_Platform.corr = ((int32_t)(TimerBase->tb_Platform.tbc_achieved - TimerBase->tb_Platform.tbc_expected))-1;
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
