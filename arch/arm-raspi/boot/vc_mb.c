/*
    Copyright © 2013-2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: VideoCore mailbox support routines
    Lang: english
*/

#include <hardware/bcm283x.h>
#include <hardware/videocore.h>

volatile unsigned int *vcmb_read(void *mb, unsigned int chan)
{
    unsigned int try = 0x2000000;
    unsigned int msg;

    if (chan <= VCMB_CHAN_MAX)
    {
        while(1)
        {
            while ((*((volatile unsigned int *)(mb + VCMB_STATUS)) & VCMB_STATUS_READREADY) != 0)
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

            if ((msg & VCMB_CHAN_MASK) == chan)
                return (volatile unsigned int *)(msg & ~VCMB_CHAN_MASK);
        }
    }
    return (volatile unsigned int *)-1;
}

void vcmb_write(void *mb, unsigned int chan, void *msg)
{
    if ((((unsigned int)msg & VCMB_CHAN_MASK) == 0) && (chan <= VCMB_CHAN_MAX))
    { 
        while ((*((volatile unsigned int *)(mb + VCMB_STATUS)) & VCMB_STATUS_WRITEREADY) != 0)
        {
            asm volatile ("mcr p15, #0, %[r], c7, c14, #0" : : [r] "r" (0) );
        }

        asm volatile ("mcr p15, #0, %[r], c7, c10, #5" : : [r] "r" (0) );

        *((volatile unsigned int *)(mb + VCMB_WRITE)) = ((unsigned int)msg | chan);
    }
}
