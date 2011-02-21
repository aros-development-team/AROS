/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id: bcpl_support.c 36685 2011-01-21 17:57:06Z twilen $

    Desc: BCPL support (backward compatibility routines for non-m68k architectures)
    Lang: english
*/

#include "dos_intern.h"

#define SEGARRAY_LENGTH 4

APTR BCPL_Setup(struct Process *me, BPTR segList, APTR entry, APTR DOSBase)
{
    IPTR *SegArray;

    /* If we have no segList, just return entry point address */
    if (!segList)
    	return entry;

    /* If we have no entry point address, make one */
    if (!entry)
    	entry = (BPTR *)BADDR(segList) + 1;

    /*
     * Allocate and fill in SegArray.
     * We don't have BCPL ABI, so this is a minimal leftover. The main thing is 3rd member
     * containing actual segList pointer. Other values are just for convenience.
     */
    SegArray = AllocVec(sizeof(IPTR) * SEGARRAY_LENGTH, MEMF_ANY);
    if (SegArray == NULL)
    	return NULL;

    SegArray[0] = SEGARRAY_LENGTH;
    SegArray[1] = -1;	/* 'system' segment */
    SegArray[2] = -2;	/* 'dosbase' segment */
    SegArray[3] = (IPTR)segList;

    me->pr_SegList = MKBADDR(SegArray);

    return entry;
}

void BCPL_Cleanup(struct Process *me)
{
    IPTR *SegArray = BADDR(me->pr_SegList);
    
    if (SegArray)
	FreeMem(SegArray, sizeof(IPTR) * SEGARRAY_LENGTH);
}
