/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id:$

    Desc: AbleICR() function.
    Lang: english
*/

#include <exec/libraries.h>
#include <proto/cia.h>
#include <proto/exec.h>
#include <hardware/cia.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>

#include "cia_intern.h"

AROS_LH1(WORD, AbleICR,
	 AROS_LHA(WORD, mask, D0),
	 struct Library *, resource, 3, Cia)
{
    AROS_LIBFUNC_INIT

    struct CIABase *CiaBase = (struct CIABase *)resource;
    volatile struct Custom *custom = (struct Custom *)0xdff000;
    UBYTE old;

    Disable();

    old = CiaBase->enable_mask;

    if (mask & 0x80)
        CiaBase->enable_mask |= mask & 0x1f;
    else
        CiaBase->enable_mask &= ~mask;

    CiaBase->hw->ciaicr = mask;
    // do we need to trigger the interrupt now?
    if (CiaBase->enable_mask & CiaBase->active_mask)
        custom->intreq = INTF_SETCLR | CiaBase->inten_mask;

    Enable();

    return old;

    AROS_LIBFUNC_EXIT
}
