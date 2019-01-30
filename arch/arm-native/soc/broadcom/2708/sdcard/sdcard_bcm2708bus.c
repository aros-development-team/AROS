/*
    Copyright � 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "sdcard_intern.h"
#include "timer.h"

void FNAME_BCMSDCBUS(BCMLEDCtrl)(int lvl)
{
    if (lvl > 0)
    {
        // Activity LED ON
        if (__arm_periiobase == BCM2835_PERIPHYSBASE)
            *(volatile unsigned int *)GPCLR0 = AROS_LONG2LE(1 << 16);
        else
            *(volatile unsigned int *)GPSET1 = AROS_LONG2LE(1 << (47 - 32));
    }
    else
    {
        // Activity LED OFF
        if (__arm_periiobase == BCM2835_PERIPHYSBASE)
            *(volatile unsigned int *)GPSET0 = AROS_LONG2LE(1 << 16);
        else
            *(volatile unsigned int *)GPCLR1 = AROS_LONG2LE(1 << (47 - 32));
    }
}

ULONG FNAME_SDCBUS(GetClockDiv)(ULONG speed, struct sdcard_Bus *bus)
{
    ULONG __BCMClkDiv;

    for (__BCMClkDiv = 0; __BCMClkDiv < V300_MAXCLKDIV; __BCMClkDiv++) {
        if ((bus->sdcb_ClockMax / (__BCMClkDiv + 1)) <= speed)
                break;
    }

    return __BCMClkDiv;
}

UBYTE FNAME_BCMSDCBUS(BCMMMIOReadByte)(ULONG reg, struct sdcard_Bus *bus)
{
    ULONG val = AROS_LE2LONG(*(volatile ULONG *)(((ULONG)bus->sdcb_IOBase + reg) & ~3));

    return (val >> ((reg & 3) << 3)) & 0xFF;
}

UWORD FNAME_BCMSDCBUS(BCMMMIOReadWord)(ULONG reg, struct sdcard_Bus *bus)
{
    ULONG val = AROS_LE2LONG(*(volatile ULONG *)(((ULONG)bus->sdcb_IOBase + reg) & ~3));

    return (val >> (((reg >> 1) & 1) << 4)) & 0xFFFF;
}

ULONG FNAME_BCMSDCBUS(BCMMMIOReadLong)(ULONG reg, struct sdcard_Bus *bus)
{
    return AROS_LE2LONG(*(volatile ULONG *)(bus->sdcb_IOBase + reg));
}

static void FNAME_BCMSDCBUS(BCM283xWriteLong)(ULONG reg, ULONG val, struct sdcard_Bus *bus)
{
    /* Bug: two SDC clock cycle delay required between successive chipset writes */
    while (sdcard_CurrentTime() < (bus->sdcb_Private + 6))
        sdcard_Udelay(1);

    *(volatile ULONG *)(bus->sdcb_IOBase + reg) = AROS_LONG2LE(val);
    bus->sdcb_Private = (IPTR)sdcard_CurrentTime();
}

void FNAME_BCMSDCBUS(BCMMMIOWriteByte)(ULONG reg, UBYTE val, struct sdcard_Bus *bus)
{
    ULONG currval = AROS_LE2LONG(*(volatile ULONG *)(((ULONG)bus->sdcb_IOBase + reg) & ~3));
    ULONG shift = (reg & 3) << 3;
    ULONG mask = 0xFF << shift;
    ULONG newval = (currval & ~mask) | (val << shift);

    FNAME_BCMSDCBUS(BCM283xWriteLong)(reg & ~3, newval, bus);
}

void FNAME_BCMSDCBUS(BCMMMIOWriteWord)(ULONG reg, UWORD val, struct sdcard_Bus *bus)
{
    ULONG currval = AROS_LE2LONG(*(volatile ULONG *)(((ULONG)bus->sdcb_IOBase + reg) & ~3));
    ULONG shift = ((reg >> 1) & 1) << 4;
    ULONG mask = 0xFFFF << shift;
    ULONG newval = (currval & ~mask) | (val << shift);

    FNAME_BCMSDCBUS(BCM283xWriteLong)(reg & ~3, newval, bus);
}

void FNAME_BCMSDCBUS(BCMMMIOWriteLong)(ULONG reg, ULONG val, struct sdcard_Bus *bus)
{
    FNAME_BCMSDCBUS(BCM283xWriteLong)(reg, val, bus);
}
