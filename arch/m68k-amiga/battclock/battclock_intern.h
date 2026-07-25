/*
    Copyright (C) 1995-2010, The AROS Development Team. All rights reserved.

    Desc: Internal data structures for battclock.resource
*/

#ifndef BATTCLOCK_INTERN_H
#define BATTCLOCK_INTERN_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif
#ifndef UTILITY_UTILITY_H
#include <utility/utility.h>
#endif

#define MSM6242B 1
#define RF5C01A 2

/*
   Battery backed up memory of the RF5C01A/RP5C01. Blocks 2 and 3 are plain
   battery RAM, 13 registers of 4 bits each: block 2 holds the high nibble of
   a byte, block 3 the low one. Registers 0 to 11 are the storage proper,
   register 12 holds a checksum over them.
*/
#define BATTMEM_REGS    13
#define BATTMEM_BYTES   12
#define BATTMEM_BITS    (BATTMEM_BYTES * 8)

struct BattClockBase
{
    struct Library bb_LibNode;
    struct UtilityBase *UtilityBase;
    volatile UBYTE *clockptr;
    UBYTE clocktype;
};

void resetbattclock(struct BattClockBase *Battclock);
UBYTE getreg(volatile UBYTE *p, UBYTE regnum);
void putreg(volatile UBYTE *p, UBYTE regnum, UBYTE v);
UBYTE getbcd(volatile UBYTE *p, UBYTE regnum);
void putbcd(volatile UBYTE *p, UBYTE regnum, UBYTE v);
void stopclock(struct BattClockBase *Battclock);
void startclock(struct BattClockBase *Battclock);
BOOL battmemload(struct BattClockBase *Battclock, UBYTE *buf);
void battmemstore(struct BattClockBase *Battclock, UBYTE *buf);

#endif //BATTCLOCK_INTERN_H
