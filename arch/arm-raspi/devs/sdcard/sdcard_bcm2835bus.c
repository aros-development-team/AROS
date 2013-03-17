/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "sdcard_intern.h"
#include "timer.h"

void FNAME_SDCBUS(BCMLEDCtrl)(int lvl)
{
    if (lvl > 0)
        *(volatile ULONG *)GPCLR0 = (1 << 16); // Turn Activity LED ON
    else
        *(volatile ULONG *)GPSET0 = (1 << 16); // Turn Activity LED OFF
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

UBYTE FNAME_SDCBUS(BCMMMIOReadByte)(ULONG reg, struct sdcard_Bus *bus)
{
    ULONG val = *(volatile ULONG *)(((ULONG)bus->sdcb_IOBase + reg) & ~3);

    return (val >> ((reg & 3) << 3)) & 0xFF;
}

UWORD FNAME_SDCBUS(BCMMMIOReadWord)(ULONG reg, struct sdcard_Bus *bus)
{
    ULONG val = *(volatile ULONG *)(((ULONG)bus->sdcb_IOBase + reg) & ~3);

    return (val >> (((reg >> 1) & 1) << 4)) & 0xFFFF;
}

ULONG FNAME_SDCBUS(BCMMMIOReadLong)(ULONG reg, struct sdcard_Bus *bus)
{
    return *(volatile ULONG *)(bus->sdcb_IOBase + reg);
}

static void FNAME_SDCBUS(BCM2835WriteLong)(ULONG reg, ULONG val, struct sdcard_Bus *bus)
{
    /* Bug: two SDC clock cycle delay required between successive chipset writes */
    while (sdcard_CurrentTime() < (bus->sdcb_Private + 6))
        sdcard_Udelay(1);

    *(volatile ULONG *)(bus->sdcb_IOBase + reg) = val;
    bus->sdcb_Private = (IPTR)sdcard_CurrentTime();
}

void FNAME_SDCBUS(BCMMMIOWriteByte)(ULONG reg, UBYTE val, struct sdcard_Bus *bus)
{
    ULONG currval = *(volatile ULONG *)(((ULONG)bus->sdcb_IOBase + reg) & ~3);
    ULONG shift = (reg & 3) << 3;
    ULONG mask = 0xFF << shift;
    ULONG newval = (currval & ~mask) | (val << shift);

    FNAME_SDCBUS(BCM2835WriteLong)(reg & ~3, newval, bus);
}

void FNAME_SDCBUS(BCMMMIOWriteWord)(ULONG reg, UWORD val, struct sdcard_Bus *bus)
{
    ULONG currval = *(volatile ULONG *)(((ULONG)bus->sdcb_IOBase + reg) & ~3);
    ULONG shift = ((reg >> 1) & 1) << 4;
    ULONG mask = 0xFFFF << shift;
    ULONG newval = (currval & ~mask) | (val << shift);

    FNAME_SDCBUS(BCM2835WriteLong)(reg & ~3, newval, bus);
}

void FNAME_SDCBUS(BCMMMIOWriteLong)(ULONG reg, ULONG val, struct sdcard_Bus *bus)
{
    FNAME_SDCBUS(BCM2835WriteLong)(reg, val, bus);
}
