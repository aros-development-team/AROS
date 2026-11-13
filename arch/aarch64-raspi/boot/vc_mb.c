/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    Desc: VideoCore mailbox support routines (AArch64)
*/

#include <exec/types.h>
#include <aros/macros.h>
#include <hardware/bcm2708.h>

#undef ARM_PERIIOBASE

#include <hardware/videocore.h>
#include <stdint.h>

#include "boot.h"
#include "io.h"

#define D(x) /* */

#define ARM_PERIIOBASE (__arm_periiobase)
extern uintptr_t __arm_periiobase;

volatile unsigned int *vcmb_read(uintptr_t mb, unsigned int chan)
{
    unsigned int try = 0x20000000;
    unsigned int msg;

    D(kprintf("[VCMB] vcmb_read(%p, %p)\n", mb, chan));

    if (chan <= VCMB_CHAN_MAX)
    {
        while(1)
        {
            while ((rd32le(mb + VCMB_STATUS) & VCMB_STATUS_READREADY) != 0)
            {
                /* Data synchronization barrier */
                asm volatile ("dsb sy" ::: "memory");

                if(try-- == 0)
                {
                    /*
                     * Return failure instead of falling through: reading
                     * VCMB_READ from an empty FIFO yields undefined data
                     * and the outer loop would spin forever.
                     */
                    kprintf("[VCMB] read timeout on channel %d\n", chan);
                    return (volatile unsigned int *)0;
                }
            }

            asm volatile ("dmb sy" ::: "memory");

            msg = rd32le(mb + VCMB_READ);
            D(kprintf("[VCMB] -> %p\n", msg));

            asm volatile ("dmb sy" ::: "memory");

            if ((msg & VCMB_CHAN_MASK) == chan)
                return (volatile unsigned int *)(uintptr_t)(msg & ~VCMB_CHAN_MASK);
        }
    }
    /* NULL on failure so `if (!msg)` checks at the call sites work */
    return (volatile unsigned int *)0;
}

void vcmb_write(uintptr_t mb, unsigned int chan, void *msg)
{
    D(kprintf("[VCMB] vcmb_write(%p, %p, %p)\n", mb, chan, msg));

    if ((((uintptr_t)msg & VCMB_CHAN_MASK) == 0) && (chan <= VCMB_CHAN_MAX))
    {
        while ((rd32le(mb + VCMB_STATUS) & VCMB_STATUS_WRITEREADY) != 0)
        {
                /* Data synchronization barrier */
                asm volatile ("dsb sy" ::: "memory");
        }

        asm volatile ("dmb sy" ::: "memory");

        wr32le(mb + VCMB_WRITE, (uint32_t)((uintptr_t)msg | chan));
    }
}

/* QEMU acknowledges every tag but leaves the response length at zero for
 * the measured clock rate, which real firmware answers. */
int vcmb_firmware_present(uintptr_t mb, volatile unsigned int *msg)
{
    msg[0] = AROS_LONG2LE(8 * 4);
    msg[1] = AROS_LONG2LE(VCTAG_REQ);
    msg[2] = AROS_LONG2LE(VCTAG_GETCLKMEASURED);
    msg[3] = AROS_LONG2LE(8);
    msg[4] = AROS_LONG2LE(4);
    msg[5] = AROS_LONG2LE(VCCLOCK_ARM);
    msg[6] = 0;
    msg[7] = 0;

    vcmb_write(mb, VCMB_PROPCHAN, (void *)msg);
    msg = vcmb_read(mb, VCMB_PROPCHAN);

    if (!msg || (msg[1] != AROS_LONG2LE(VCTAG_RESP)))
        return 0;

    /* Response length of 8 bytes plus a plausible rate. */
    return ((AROS_LE2LONG(msg[4]) & 0x7fffffff) >= 8) && (AROS_LE2LONG(msg[6]) != 0);
}
