/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: FreePotBits() function.
    Lang: english
*/

#include <proto/potgo.h>
#include <hardware/custom.h>

#include "potgo_intern.h"

AROS_LH1(void, FreePotBits,
	 AROS_LHA(UWORD, allocated, D0),
	 struct PotgoBase *, PotgoBase, 2, Potgo)
{
    AROS_LIBFUNC_INIT

	volatile struct Custom *custom = (struct Custom*)0xdff000;

	PotgoBase->allocated &= ~allocated;
	PotgoBase->data &= ~allocated;
	custom->potgo = PotgoBase->data;

    AROS_LIBFUNC_EXIT
}
