/*
    Copyright � 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
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

    if (chan <= VCMB_CHAN_MAX)
    {
        while(1)
        {
            ObtainSemaphore(&MBoxBase->mbox_Sem);

            while ((MBoxStatus(mb) & VCMB_STATUS_READREADY) != 0)
            {
                /* Data synchronization barrier */
                asm volatile ("mcr p15, 0, %[r], c7, c10, 4" : : [r] "r" (0) );

                if(try-- == 0)
                {
                    break;
                }
            }
            asm volatile ("mcr p15, 0, %[r], c7, c10, 5" : : [r] "r" (0) );

            msg = AROS_LE2LONG(*((volatile unsigned int *)(mb + VCMB_READ)));

            asm volatile ("mcr p15, 0, %[r], c7, c10, 5" : : [r] "r" (0) );

            ReleaseSemaphore(&MBoxBase->mbox_Sem);

            if ((msg & VCMB_CHAN_MASK) == chan)
                return (volatile unsigned int *)(msg & ~VCMB_CHAN_MASK);
        }
    }
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
        ULONG length = ((ULONG *)msg)[0];

        void *phys_addr = CachePreDMA(msg, &length, DMA_ReadFromRAM);

        ObtainSemaphore(&MBoxBase->mbox_Sem);

        while ((MBoxStatus(mb) & VCMB_STATUS_WRITEREADY) != 0)
        {
            /* Data synchronization barrier */
            asm volatile ("mcr p15, 0, %[r], c7, c10, 4" : : [r] "r" (0) );
        }

        asm volatile ("mcr p15, 0, %[r], c7, c10, 5" : : [r] "r" (0) );

        *((volatile unsigned int *)(mb + VCMB_WRITE)) = AROS_LONG2LE(((unsigned int)phys_addr | chan));

        ReleaseSemaphore(&MBoxBase->mbox_Sem);
    }

    AROS_LIBFUNC_EXIT
}

ADD2INITLIB(mbox_init, 0)
