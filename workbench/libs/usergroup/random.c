/*
 * random.c --- random numbers
 *
 * Original Author: ppessi <Pekka.Pessi@hut.fi>
 *
 * Based upon usergroup.library from AmiTCP/IP.
 *
 * Copyright © 2025 The AROS Dev Team.
 * Copyright © 1993 AmiTCP/IP Group, <AmiTCP-Group@hut.fi>
 *                  Helsinki University of Technology, Finland.
 */

#include <exec/types.h>
#include <exec/io.h>
#include <devices/timer.h>
#include <proto/timer.h>

#if defined(__AROS__)
extern struct Device *TimerBase;
#else
extern struct Library *TimerBase;
#endif

/****i* random.module/LRandom ******************************************
*
*   NAME
*	LRandom -- Random number generator
*
*   SYNOPSIS
*    	value = LRandom()
*
*	LONG InitLRandom(void)
*
*   FUNCTION
*      Generates a random long integer. The random number generator
*      must have been initialized before calling LRandom() or strange
*      things will happen.
*
*      LRandom() generates fairly good random numbers, all bits in the
*      long word seem to be useful.
*
*   BUGS
*      None known.
*
*   SEE ALSO
*	InitLRandom, timer.device/GetSysTime()
*
******************************************************************************
*/

static union random_seed {
    struct timeval time;
    ULONG          longs[2];
    UWORD          words[4];
} seed;

static const UWORD random_offset[4] = { 0x7823, 0xab34, 0x93b4, 0x7673 };

static const UWORD random_eor[4] = { 0xc97d, 0x6988, 0x32e9, 0x8487 };

ULONG LRandom(void)
{
    ULONG carry = 0;
    int  i;

    for (i = 3; i >= 0; i--) {
        carry += seed.words[i]*34639L + random_offset[i];
        seed.words[i] = carry ^ random_eor[i];
        carry >>= 16;
    }

    return (ULONG)(seed.words[3] << 24) + (seed.words[2]<<16) +
           (seed.words[1] << 8) + seed.words[0];
}

/****i* random.module/LRandomInit ******************************************
*
*   NAME
*	LRandomInit -- initialize LRandom generator
*
*   SYNOPSIS
*    	error = LRandomInit();
*
*	int LRandomInit(void);
*
*   FUNCTION
*      Initializes the random number generator with the current time.
*
*   NOTES
*      This function uses GetSysTime() to get the initial seed for the
*      random number generator. Thus it needs the V36 timer.device.
*
*   RETURN VALUE
*      Nonzero after initialization error.
*
*   BUGS
*      None known.
*
*   SEE ALSO
*	LRandom, timer.device/GetSysTime()
*
******************************************************************************
*/

int LRandomInit(void)
{
    if (TimerBase) {
        GetSysTime(&seed.time);
        LRandom();
        LRandom();
        LRandom();
        LRandom();
        LRandom();
        LRandom();
        LRandom();
        LRandom();
        return 0;
    }
    return -1;
}
