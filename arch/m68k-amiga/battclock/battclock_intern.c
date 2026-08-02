/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/libcall.h>

#include "battclock_intern.h"

UBYTE getreg(volatile UBYTE *p, UBYTE regnum)
{
    return p[regnum * 4 + 3] & 15;
}
void putreg(volatile UBYTE *p, UBYTE regnum, UBYTE v)
{
    p[regnum * 4 + 3] = v;
}
UBYTE getbcd(volatile UBYTE *p, UBYTE regnum)
{
    return getreg(p, regnum + 1) * 10 + getreg(p, regnum);
}
void putbcd(volatile UBYTE *p, UBYTE regnum, UBYTE v)
{
    putreg(p, regnum + 1, v / 10);
    putreg(p, regnum, v % 10);
}

void stopclock(struct BattClockBase *Battclock)
{
    volatile UBYTE *p = Battclock->clockptr;
    if (Battclock->clocktype == MSM6242B) {
        putreg(p, 0xf, 4 + 2);
    } else if (Battclock->clocktype == RF5C01A) {
        putreg(p, 0xd, 0);
    }
}
void startclock(struct BattClockBase *Battclock)
{
    volatile UBYTE *p = Battclock->clockptr;
    if (Battclock->clocktype == MSM6242B) {
        putreg(p, 0xf, 4);
    } else if (Battclock->clocktype == RF5C01A) {
        putreg(p, 0xd, 8);
    }
}

/*
   Checksum over the battery memory, a nibble wide CRC seeded with 0xff.
   It has to match the one the Amiga ROM uses byte for byte, or settings
   written under Kickstart would be discarded here and the other way round.
*/
static UBYTE battmemsum(const UBYTE *buf)
{
    static const UBYTE tab[16] =
    {
        0x00, 0x57, 0xae, 0xf9, 0x0b, 0x5c, 0xa5, 0xf2,
        0x16, 0x41, 0xb8, 0x7f, 0x1d, 0x4a, 0xb3, 0xe4
    };
    UBYTE sum = 0xff;
    UBYTE i, t;

    for (i = 0; i < BATTMEM_BYTES * 2; i++)
    {
        /* High nibble of each byte first, then the low one. */
        t = ((i & 1) ? (buf[i / 2] << 4) : (buf[i / 2] & 0xf0)) ^ sum;
        t = (t >> 4) | (t << 4);
        sum = (t & 0xf0) ^ tab[t & 0x0f];
    }

    return sum;
}

static void battmemread(struct BattClockBase *Battclock, UBYTE *buf)
{
    volatile UBYTE *p = Battclock->clockptr;
    UBYTE i;

    putreg(p, 0xd, 8 | 2); // timer en, block 2
    for (i = 0; i < BATTMEM_REGS; i++)
        buf[i] = getreg(p, i) << 4;
    putreg(p, 0xd, 8 | 3); // timer en, block 3
    for (i = 0; i < BATTMEM_REGS; i++)
        buf[i] |= getreg(p, i);
    putreg(p, 0xd, 8); // timer en, block 0
}

static void battmemwrite(struct BattClockBase *Battclock, const UBYTE *buf)
{
    volatile UBYTE *p = Battclock->clockptr;
    UBYTE i;

    putreg(p, 0xd, 8 | 2); // timer en, block 2
    for (i = 0; i < BATTMEM_REGS; i++)
        putreg(p, i, buf[i] >> 4);
    putreg(p, 0xd, 8 | 3); // timer en, block 3
    for (i = 0; i < BATTMEM_REGS; i++)
        putreg(p, i, buf[i] & 0x0f);
    putreg(p, 0xd, 8); // timer en, block 0
}

/*
   Fetch the battery memory into buf, which holds BATTMEM_REGS bytes. If the
   checksum does not match, the battery lost its contents: clear it out, so
   that the AMNESIA bits read back as zero and callers can tell.
   Returns FALSE if this clock has no battery memory at all.
*/
BOOL battmemload(struct BattClockBase *Battclock, UBYTE *buf)
{
    UBYTE i;

    if (!Battclock->clockptr || Battclock->clocktype != RF5C01A)
        return FALSE;

    battmemread(Battclock, buf);

    if (buf[BATTMEM_BYTES] != battmemsum(buf))
    {
        for (i = 0; i < BATTMEM_REGS; i++)
            buf[i] = 0;
        buf[BATTMEM_BYTES] = battmemsum(buf);
        battmemwrite(Battclock, buf);
    }

    return TRUE;
}

void battmemstore(struct BattClockBase *Battclock, UBYTE *buf)
{
    buf[BATTMEM_BYTES] = battmemsum(buf);
    battmemwrite(Battclock, buf);
}

void resetbattclock(struct BattClockBase *Battclock)
{
    volatile UBYTE *p = Battclock->clockptr;
    UBYTE i, j;

    if (!p)
        return;

    if (Battclock->clocktype == MSM6242B) {
        putreg(p, 0xd, 0);
        putreg(p, 0xe, 0);
        putreg(p, 0xf, 2);
        for (i = 0; i < 12; i++)
            putreg(p, i, 0);
        putreg(p, 0xf, 7); // reset
        putreg(p, 0xf, 4); // leave 24h on
    } else if (Battclock->clocktype == RF5C01A) {
        putreg(p, 0xd, 0); // stop
        putreg(p, 0xe, 0);
        /*
           Only blocks 0 and 1 are the clock. Blocks 2 and 3 are battery
           backed up memory holding settings that belong to the user -
           clearing them here would throw those away on every clock reset.
        */
        for (j = 0; j < 2; j++) {
            putreg(p, 0xd, j);
            for (i = 0; i < 12; i++)
                putreg(p, i, 0);
        }
        putreg(p, 0xd, 0);
        putreg(p, 0xf, 3); // reset
        putreg(p, 0xf, 0); // reset off
        putreg(p, 0xd, 8); // timer en
    }
}
