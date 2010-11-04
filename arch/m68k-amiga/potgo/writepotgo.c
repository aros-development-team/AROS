/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: WritePotgo() function.
    Lang: english
*/

#include <proto/potgo.h>
#include <hardware/custom.h>

#include "potgo_intern.h"

AROS_LH2(void, WritePotgo,
	 AROS_LHA(UWORD, word, D0),
	 AROS_LHA(UWORD, mask, D1),
	 struct PotgoBase *, PotgoBase, 3, Potgo)
{
    AROS_LIBFUNC_INIT

	volatile struct Custom *custom = (struct Custom*)0xdff000;

	word &= 0xff01;
	PotgoBase->data |= (word & ~1) & mask;
	PotgoBase->data &= word | (~mask);

	custom->potgo = PotgoBase->data | (word & 1);

    AROS_LIBFUNC_EXIT
}
