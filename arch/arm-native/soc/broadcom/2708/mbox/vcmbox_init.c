/*
    Copyright � 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/vcmbox.h>

#include <hardware/bcm283x.h>
#include <hardware/videocore.h>

#include "vcmbox_private.h"


static int vcmbox_init(struct VCMBoxBase *VCMBoxBase)
{
    int retval = TRUE;

    D(bug("[VCMBox] vcmbox_init()\n"));

    InitSemaphore(&VCMBoxBase->vcmb_Sem);

    D(bug("[VCMBox] vcmbox_init: Initialised Semaphore @ 0x%p\n", &VCMBoxBase->vcmb_Sem));

    return retval;
}

AROS_LH1(unsigned int, VCMBoxStatus,
		AROS_LHA(void *, mb, A0),
		struct VCMBoxBase *, VCMBoxBase, 1, Vcmbox)
{
    AROS_LIBFUNC_INIT

    D(bug("[VCMBox] VCMBoxStatus(0x%p)\n", mb));

    return *((volatile unsigned int *)(mb + VCMB_STATUS));

    AROS_LIBFUNC_EXIT
}

AROS_LH2(volatile unsigned int *, VCMBoxRead,
		AROS_LHA(void *, mb, A0),
		AROS_LHA( unsigned int, chan, D0),
		struct VCMBoxBase *, VCMBoxBase, 2, Vcmbox)
{
    AROS_LIBFUNC_INIT

    unsigned int try = 0x2000000;
    unsigned int msg;

    D(bug("[VCMBox] VCMBoxRead(chan %d @ 0x%p)\n", chan, mb));

    if (chan <= VCMB_CHAN_MAX)
    {
        while(1)
        {
            ObtainSemaphore(&VCMBoxBase->vcmb_Sem);
            APTR ssp = SuperState();
            while ((VCMBoxStatus(mb) & VCMB_STATUS_READREADY) != 0)
            {
                /* Data synchronization barrier */
                asm volatile ("mcr p15, 0, %[r], c7, c10, 4" : : [r] "r" (0) );

                if(try-- == 0)
                {
                    break;
                }
            }
            asm volatile ("mcr p15, 0, %[r], c7, c10, 5" : : [r] "r" (0) );

            msg = *((volatile unsigned int *)(mb + VCMB_READ));
            
            asm volatile ("mcr p15, 0, %[r], c7, c10, 5" : : [r] "r" (0) );
            UserState(ssp);
            ReleaseSemaphore(&VCMBoxBase->vcmb_Sem);

            if ((msg & VCMB_CHAN_MASK) == chan)
                return (volatile unsigned int *)(msg & ~VCMB_CHAN_MASK);
        }
    }
    return (volatile unsigned int *)-1;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, VCMBoxWrite,
		AROS_LHA(void *, mb, A0),
		AROS_LHA( unsigned int, chan, D0),
		AROS_LHA(void *, msg, A1),
		struct VCMBoxBase *, VCMBoxBase, 3, Vcmbox)
{
    AROS_LIBFUNC_INIT

    D(bug("[VCMB] VCMBWrite(chan %d @ 0x%p, msg @ 0x%p)\n", chan, mb, msg));

    if ((((unsigned int)msg & VCMB_CHAN_MASK) == 0) && (chan <= VCMB_CHAN_MAX))
    { 
        ObtainSemaphore(&VCMBoxBase->vcmb_Sem);
        APTR ssp = SuperState();
        while ((VCMBoxStatus(mb) & VCMB_STATUS_WRITEREADY) != 0)
        {
            /* Data synchronization barrier */
            asm volatile ("mcr p15, 0, %[r], c7, c10, 4" : : [r] "r" (0) );
        }

        asm volatile ("mcr p15, 0, %[r], c7, c10, 5" : : [r] "r" (0) );

        *((volatile unsigned int *)(mb + VCMB_WRITE)) = ((unsigned int)msg | chan);
        UserState(ssp);
        ReleaseSemaphore(&VCMBoxBase->vcmb_Sem);
    }

    AROS_LIBFUNC_EXIT
}

ADD2INITLIB(vcmbox_init, 0)
