/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id:$

    Desc: AddICRVector() function.
    Lang: english
*/

#include <exec/interrupts.h>
#include <proto/cia.h>
#include <proto/exec.h>

#include "cia_intern.h"

AROS_LH2(struct Interrupt *, AddICRVector,
	 AROS_LHA(LONG, iCRBit, D0),
	 AROS_LHA(struct Interrupt *, interrupt, A1),
	 struct Library *, resource, 1, Cia)
{
    AROS_LIBFUNC_INIT

    struct CIABase *CiaBase = (struct CIABase *)resource;
    struct Interrupt *old;

    /* 68k lowlevel library calls have garbage in upper word */
    iCRBit = (WORD)iCRBit;
    Disable();
    old = CiaBase->Vectors[iCRBit];
    if (!old) {
        CiaBase->Vectors[iCRBit] = interrupt;
        AbleICR(resource, 0x80 | (1 << iCRBit));
    }
    Enable();
    return old;

    AROS_LIBFUNC_EXIT
}
