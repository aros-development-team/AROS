/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.

    Desc: Internal data structures for battclock.resource, PC hardware version
*/

#include <exec/libraries.h>
#include <exec/semaphores.h>

struct BattClockBase
{
    struct Library bb_LibNode;	/* battclock.resource originally had struct Library in its base */
    struct SignalSemaphore sem;	/* Semaphore used for single-threading hardware access		*/
    UBYTE century;		/* Century byte offset						*/
};
