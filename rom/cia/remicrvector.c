/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id:$

    Desc: RemICRVector() function.
    Lang: english
*/

#include <exec/interrupts.h>
#include <proto/cia.h>
#include <proto/exec.h>

#include "cia_intern.h"

AROS_LH2(void, RemICRVector,
	 AROS_LHA(LONG, iCRBit, D0),
	 AROS_LHA(struct Interrupt *, interrupt, A1),
	 struct Library *, resource, 7, Cia)
{
    AROS_LIBFUNC_INIT

    struct CIABase *CiaBase = (struct CIABase *)resource;

    AbleICR(resource, 1 << iCRBit);

    Disable();
    if (CiaBase->Vectors[iCRBit] == interrupt)
        CiaBase->Vectors[iCRBit] = NULL;
    Enable();

    AROS_LIBFUNC_EXIT
}
