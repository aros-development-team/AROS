/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id: $

    Desc: Amiga(tm) chipset control macros
    Lang: english
*/

#ifdef AROS_ARCH_amiga

#include <hardware/custom.h>

static inline void CUSTOM_ENABLE(ULONG intNumber)
{
    if (intNumber < INTB_INTEN)
    {
        volatile struct Custom *custom = (struct Custom *)(void **)0xdff000;

	custom->intena = (UWORD)(INTF_SETCLR|(1L<<intNumber));
    }
}

static inline void CUSTOM_DISABLE(ULONG intNumber, struct List *list)
{
    if (intNumber < INTB_INTEN)
    {
        /* disable interrupts if there are no more nodes on the list */
        if (list->lh_TailPred == (struct Node *)list)
        {
	    volatile struct Custom *custom = (struct Custom *)(void **)0xdff000;

	    custom->intena = (UWORD)((1<<intNumber));
	}
    }
}

static inline void CUSTOM_ACK(UWORD intBit)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;

    custom->intena = intBit;
    custom->intreq = intBit;
}

static inline void CUSTOM_CAUSE(UWORD intBit)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;

    custom->intreq = INTF_SETCLR | intBit;
}

#else

struct Custom;

#define CUSTOM_ENABLE(intNumber)
#define CUSTOM_DISABLE(intNumber, list)
#define CUSTOM_ACK(intBit)
#define CUSTOM_CAUSE(intBit)

#endif
