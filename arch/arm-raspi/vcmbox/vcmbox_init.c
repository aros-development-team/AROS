/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <proto/openfirmware.h>
#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/vcmbox.h>

#include <hardware/vcmbox.h>

#include "vcmbox_private.h"


static int vcmbox_init(struct VCMBoxBase *VCMBoxBase)
{
    int retval = TRUE;

    D(bug("[VCMBox] vcmbox_init()\n"));

    return retval;
}

AROS_LH1(unsigned int, VCMBoxStatus,
		AROS_LHA(void *, mb, A0),
		struct VCMBoxBase *, VCMBoxBase, 1, Vcmb)
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

    if (chan <= VCMB_CHANS)
    {
        while(1)
        {
            while ((VCMBoxStatus(mb) & VCMB_STATUS_READREADY) != 0)
            {
                asm volatile ("mcr p15, #0, %[r], c7, c14, #0" : : [r] "r" (0) );

                if(try-- == 0)
                {
                    break;
                }
            }
            asm volatile ("mcr p15, #0, %[r], c7, c10, #5" : : [r] "r" (0) );

            msg = *((volatile unsigned int *)(mb + VCMB_READ));
            
            asm volatile ("mcr p15, #0, %[r], c7, c10, #5" : : [r] "r" (0) );

            if ((msg & VCMB_CHANMASK) == chan)
                return (volatile unsigned int *)(msg & ~VCMB_CHANMASK);
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

    if ((((unsigned int)msg & VCMB_CHANMASK) == 0) && (chan <= VCMB_CHANS))
    { 
        while ((VCMBoxStatus(mb) & VCMB_STATUS_WRITEREADY) != 0)
        {
            asm volatile ("mcr p15, #0, %[r], c7, c14, #0" : : [r] "r" (0) );
        }

        asm volatile ("mcr p15, #0, %[r], c7, c10, #5" : : [r] "r" (0) );

        *((volatile unsigned int *)(mb + VCMB_WRITE)) = ((unsigned int)msg | chan);
    }

    AROS_LIBFUNC_EXIT
}

ADD2INITLIB(vcmbox_init, 0)
