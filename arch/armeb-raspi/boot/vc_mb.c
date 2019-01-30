/*
    Copyright � 2013-2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: VideoCore mailbox support routines
    Lang: english
*/

#include <exec/types.h>
#include <aros/macros.h>
#include <hardware/bcm2708.h>

#undef ARM_PERIIOBASE

#include <hardware/videocore.h>
#include <stdint.h>

#include "io.h"

#define ARM_PERIIOBASE (__arm_periiobase)
extern uint32_t __arm_periiobase;

volatile unsigned int *vcmb_read(uintptr_t mb, unsigned int chan)
{
    unsigned int try = 0x20000000;
    unsigned int msg;

    if (chan <= VCMB_CHAN_MAX)
    {
    	while(1)
        {
            while ((rd32le(mb + VCMB_STATUS) & VCMB_STATUS_READREADY) != 0)
            {
            	/* Data synchronization barrier */
            	asm volatile ("mcr p15, 0, %[r], c7, c10, 4" : : [r] "r" (0) );

                if(try-- == 0)
                {
                    break;
                }
            }

            asm volatile ("mcr p15, #0, %[r], c7, c10, #5" : : [r] "r" (0) );

            msg = rd32le(mb + VCMB_READ);
            
            asm volatile ("mcr p15, #0, %[r], c7, c10, #5" : : [r] "r" (0) );

            if ((msg & VCMB_CHAN_MASK) == chan)
                return (volatile unsigned int *)(msg & ~VCMB_CHAN_MASK);
        }
    }
    return (volatile unsigned int *)-1;
}

void vcmb_write(uintptr_t mb, unsigned int chan, void *msg)
{
    if ((((unsigned int)msg & VCMB_CHAN_MASK) == 0) && (chan <= VCMB_CHAN_MAX))
    { 
        while ((rd32le(mb + VCMB_STATUS) & VCMB_STATUS_WRITEREADY) != 0)
        {
        	/* Data synchronization barrier */
        	asm volatile ("mcr p15, 0, %[r], c7, c10, 4" : : [r] "r" (0) );
        }

        asm volatile ("mcr p15, #0, %[r], c7, c10, #5" : : [r] "r" (0) );

        wr32le(mb + VCMB_WRITE, (uintptr_t)msg | chan);
    }
}
