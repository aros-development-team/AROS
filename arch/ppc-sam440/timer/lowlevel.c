#include <asm/amcc440.h>
#include "timer_intern.h"
#include "../kernel/syscall.h"

#include <aros/asmcall.h>
#include <hardware/intbits.h>
#include <exec/execbase.h>

#include <inttypes.h>
#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/kernel.h>

uint32_t tick2usec(uint32_t tick)
{
    uint32_t retval;
    uint64_t tmp = ((uint64_t)tick) * 6442451;

    retval = (tmp + 0x7fffffff) >> 32;

    return retval;
}

uint32_t usec2tick(uint32_t usec)
{
    uint32_t retval;
    uint64_t tmp = ((uint64_t)usec) * 22369621311ULL;

    retval = tmp >> 25;

    return retval;
}

void EClockUpdate(struct TimerBase *TimerBase)
{
    union {
        uint32_t u32[2];
        uint64_t u64;
    } timebase;
    
    uint64_t diff;
    
    Disable();
    
    do {
        timebase.u32[0] = rdspr(TBUU);
        timebase.u32[1] = rdspr(TBLU);
    } while(timebase.u32[0] != rdspr(TBUU));
    
    diff = timebase.u64 - TimerBase->tb_ticks_total;
    TimerBase->tb_ticks_total = timebase.u64;
    
    TimerBase->tb_ticks_sec += diff;
    TimerBase->tb_ticks_elapsed += diff;
    
    if (TimerBase->tb_ticks_sec >= 666666666)
    {
        TimerBase->tb_ticks_sec -= 666666666;
        TimerBase->tb_CurrentTime.tv_secs++;
    }
    
    if (TimerBase->tb_ticks_elapsed >= 666666666)
    {
        TimerBase->tb_ticks_elapsed -= 666666666;
        TimerBase->tb_Elapsed.tv_secs++;
    }

    TimerBase->tb_Elapsed.tv_micro = tick2usec(TimerBase->tb_ticks_elapsed);
    TimerBase->tb_CurrentTime.tv_micro = tick2usec(TimerBase->tb_ticks_sec);
    
    Enable();
}

void EClockSet(struct TimerBase *TimerBase)
{
    TimerBase->tb_ticks_sec = usec2tick(TimerBase->tb_CurrentTime.tv_micro);
}

void TimerSetup(struct TimerBase *TimerBase)
{
    void *KernelBase = rdspr(SPRG4U);
    uint32_t delay = 13333333;  /* 50Hz in worst case */
    struct timeval time;
    struct timerequest *tr;
    char super = 0;
    
    tr = (struct timerequest *)GetHead(&TimerBase->tb_Lists[TL_WAITVBL]);

    if (tr)
    {    
        time.tv_micro = tr->tr_time.tv_micro;
        time.tv_secs  = tr->tr_time.tv_secs;

        EClockUpdate(TimerBase);
        SubTime(&time, &TimerBase->tb_CurrentTime);
    
        if ((LONG)time.tv_secs < 0)
        {
            delay = 0;
        }
        else if (time.tv_secs == 0)
        {
            if (time.tv_micro < 20000)
            {
                delay = usec2tick(time.tv_micro);
            }
        }
    }
    
    tr = (struct timerequest *)GetHead(&TimerBase->tb_Lists[TL_VBLANK]);

    if (tr)
    {    
        time.tv_micro = tr->tr_time.tv_micro;
        time.tv_secs  = tr->tr_time.tv_secs;

        EClockUpdate(TimerBase);
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
    
    if (delay < 2) delay = 2;
    
    if (!(super = KrnIsSuper()))
        asm volatile("li %%r3,%0; sc"::"i"(SC_SUPERSTATE):"memory","r3");
    
    /* Program the delay */
    wrspr(DEC, delay);
    /* Disable auto reload and enable decrementer */
    wrspr(TCR, (rdspr(TCR) & ~TCR_ARE) | TCR_DIE);

    if (!super)
        wrmsr(rdmsr() | (MSR_PR));
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

void DecrementerHandler(struct TimerBase *TimerBase, struct ExecBase *SysBase)
{
    struct timerequest *tr, *next;

    EClockUpdate(TimerBase);

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

    TimerSetup(TimerBase);
    wrspr(TSR, TSR_DIS);
}
