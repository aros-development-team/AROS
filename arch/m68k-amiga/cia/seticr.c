/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SetICR() function.
    Lang: english
*/

#include <exec/libraries.h>
#include <proto/cia.h>
#include <proto/exec.h>
#include <hardware/cia.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>

#include "cia_intern.h"

AROS_LH1(WORD, SetICR,
	 AROS_LHA(WORD, mask, D0),
	 struct Library *, resource, 4, Cia)
{
    AROS_LIBFUNC_INIT

    struct CIABase *CiaBase = (struct CIABase *)resource;
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    WORD old;
     
    Disable();
     
    // I think this needs to return interrupt=active status
    // if called inside CIA interrupt handler
    CiaBase->active_mask |= CiaBase->hw->ciaicr & 0x1f;
    old = CiaBase->active_mask | CiaBase->executing_mask;
    if (mask & 0x80)
        CiaBase->active_mask |= mask & 0x1f;
    else
        CiaBase->active_mask &= ~mask;

    // do we need to trigger the interrupt now?
    if (CiaBase->enable_mask & CiaBase->active_mask)
        custom->intreq = INTF_SETCLR | CiaBase->inten_mask;

    Enable();

    return old;

    AROS_LIBFUNC_EXIT
}
