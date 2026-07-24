/*
    Copyright (C) 2013-2015, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0

#include <aros/macros.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/mbox.h>

#include <hardware/bcm2708.h>
#include <hardware/videocore.h>

#include "mbox_private.h"


static int mbox_init(struct MBoxBase *MBoxBase)
{
    int retval = TRUE;

    D(bug("[MBox] mbox_init()\n"));

    InitSemaphore(&MBoxBase->mbox_Sem);

    D(bug("[MBox] mbox_init: Initialised Semaphore @ 0x%p\n", &MBoxBase->mbox_Sem));

    return retval;
}

volatile unsigned int *mbox_call_locked(struct MBoxBase *MBoxBase, void *mb,
    unsigned int chan, void *msg)
{
    unsigned int try = 0x2000000;
    unsigned int reply;
    ULONG length;
    void *phys_addr;

    if ((((unsigned int)msg & VCMB_CHAN_MASK) != 0) || (chan > VCMB_CHAN_MAX))
        return (volatile unsigned int *)-1;

    length = AROS_LE2LONG(((ULONG *)msg)[0]);
    phys_addr = CachePreDMA(msg, &length, DMA_ReadFromRAM);

    ObtainSemaphore(&MBoxBase->mbox_Sem);

    {
        unsigned int wtry = 0x2000000;
        while ((MBoxStatus(mb) & VCMB_STATUS_WRITEREADY) != 0)
        {
            __asm__ __volatile__("dsb sy" ::: "memory");
            if (--wtry == 0)
            {
                ReleaseSemaphore(&MBoxBase->mbox_Sem);
                return (volatile unsigned int *)-1;
            }
        }
    }

    __asm__ __volatile__("dmb sy" ::: "memory");
    *((volatile unsigned int *)(mb + VCMB_WRITE)) =
        AROS_LONG2LE(((unsigned int)phys_addr | chan));

    /* Drain the inbox until either our own response shows up (matched by
     * channel AND by physical address — the address check distinguishes our
     * submission from any other caller queued on the same channel) or the
     * timeout budget is exhausted.*/
    while (try > 0)
    {
        if ((MBoxStatus(mb) & VCMB_STATUS_READREADY) != 0)
        {
            __asm__ __volatile__("dsb sy" ::: "memory");
            try--;
            continue;
        }

        __asm__ __volatile__("dmb sy" ::: "memory");
        reply = AROS_LE2LONG(*((volatile unsigned int *)(mb + VCMB_READ)));
        __asm__ __volatile__("dmb sy" ::: "memory");

        if ((reply & VCMB_CHAN_MASK) == chan)
        {
            uint32_t *addr = (uint32_t *)(reply & ~VCMB_CHAN_MASK);
            uint32_t len = AROS_LE2LONG(addr[0]);

            CacheClearE(addr, len, CACRF_InvalidateD);

            if ((void *)addr == phys_addr)
            {
                ReleaseSemaphore(&MBoxBase->mbox_Sem);
                return (volatile unsigned int *)msg;
            }
        }
        /* Wrong channel or wrong address — drop the message and keep
         * draining until we find ours, or 'try' reaches zero. */
    }

    ReleaseSemaphore(&MBoxBase->mbox_Sem);
    return (volatile unsigned int *)-1;
}

AROS_LH1(unsigned int, MBoxStatus,
                AROS_LHA(void *, mb, A0),
                struct MBoxBase *, MBoxBase, 1, Mbox)
{
    AROS_LIBFUNC_INIT

    D(bug("[MBox] MBoxStatus(0x%p)\n", mb));

    return AROS_LE2LONG(*((volatile unsigned int *)(mb + VCMB_STATUS)));

    AROS_LIBFUNC_EXIT
}

AROS_LH2(volatile unsigned int *, MBoxRead,
                AROS_LHA(void *, mb, A0),
                AROS_LHA( unsigned int, chan, D0),
                struct MBoxBase *, MBoxBase, 2, Mbox)
{
    AROS_LIBFUNC_INIT

    unsigned int try = 0x2000000;
    unsigned int msg;

    D(bug("[MBox] MBoxRead(chan %d @ 0x%p)\n", chan, mb));

    if (chan > VCMB_CHAN_MAX)
        return (volatile unsigned int *)-1;

    ObtainSemaphore(&MBoxBase->mbox_Sem);

    /* Drain until we find a message for our channel or the timeout
     * budget runs out. */
    while (try > 0)
    {
        if ((MBoxStatus(mb) & VCMB_STATUS_READREADY) != 0)
        {
            asm volatile ("dsb sy" ::: "memory");
            try--;
            continue;
        }

        asm volatile ("dmb sy" ::: "memory");
        msg = AROS_LE2LONG(*((volatile unsigned int *)(mb + VCMB_READ)));
        asm volatile ("dmb sy" ::: "memory");

        if ((msg & VCMB_CHAN_MASK) == chan)
        {
            uint32_t *addr = (uint32_t *)(msg & ~VCMB_CHAN_MASK);
            uint32_t len = AROS_LE2LONG(addr[0]);

            ReleaseSemaphore(&MBoxBase->mbox_Sem);

            CacheClearE(addr, len, CACRF_InvalidateD);

            return (volatile unsigned int *)(addr);
        }
        /* Wrong channel — drop and keep draining. */
    }

    ReleaseSemaphore(&MBoxBase->mbox_Sem);
    return (volatile unsigned int *)-1;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, MBoxWrite,
                AROS_LHA(void *, mb, A0),
                AROS_LHA( unsigned int, chan, D0),
                AROS_LHA(void *, msg, A1),
                struct MBoxBase *, MBoxBase, 3, Mbox)
{
    AROS_LIBFUNC_INIT

    D(bug("[MBOX] MBoxWrite(chan %d @ 0x%p, msg @ 0x%p)\n", chan, mb, msg));

    if ((((unsigned int)msg & VCMB_CHAN_MASK) == 0) && (chan <= VCMB_CHAN_MAX))
    {
        ULONG length = AROS_LE2LONG(((ULONG *)msg)[0]);

        void *phys_addr = CachePreDMA(msg, &length, DMA_ReadFromRAM);

        ObtainSemaphore(&MBoxBase->mbox_Sem);

        while ((MBoxStatus(mb) & VCMB_STATUS_WRITEREADY) != 0)
        {
            /* Data synchronization barrier */
            asm volatile ("dsb sy" ::: "memory");
        }

        asm volatile ("dmb sy" ::: "memory");

        *((volatile unsigned int *)(mb + VCMB_WRITE)) = AROS_LONG2LE(((unsigned int)phys_addr | chan));

        ReleaseSemaphore(&MBoxBase->mbox_Sem);
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH3(volatile unsigned int *, MBoxCall,
                AROS_LHA(void *, mb, A0),
                AROS_LHA(unsigned int, chan, D0),
                AROS_LHA(void *, msg, A1),
                struct MBoxBase *, MBoxBase, 4, Mbox)
{
    AROS_LIBFUNC_INIT

    D(bug("[MBOX] MBoxCall(chan %d @ 0x%p, msg @ 0x%p)\n", chan, mb, msg));

    if (msg == NULL)
        return (volatile unsigned int *)-1;

    return mbox_call_locked(MBoxBase, mb, chan, msg);

    AROS_LIBFUNC_EXIT
}

ADD2INITLIB(mbox_init, 0)
