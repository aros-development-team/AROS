/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.

    Desc: RemICRVector() function.
*/

#include <exec/interrupts.h>
#include <proto/cia.h>
#include <proto/exec.h>

#include "cia_intern.h"

AROS_LH2(void, RemICRVector,
         AROS_LHA(LONG, iCRBit, D0),
         AROS_LHA(struct Interrupt *, interrupt, A1),
         struct Library *, resource, 2, Cia)
{
    AROS_LIBFUNC_INIT

    struct CIABase *CiaBase = (struct CIABase *)resource;

    /* 68k lowlevel library calls have garbage in upper word */
    iCRBit = (WORD)iCRBit;
    AbleICR(resource, 1 << iCRBit);
    Disable();
    if (CiaBase->Vectors[iCRBit].iv_Node == interrupt) {
        CiaBase->Vectors[iCRBit].iv_Node = NULL;
        CiaBase->Vectors[iCRBit].iv_Code = NULL;
        CiaBase->Vectors[iCRBit].iv_Data = NULL;
    }
    Enable();
    AROS_LIBFUNC_EXIT
}
