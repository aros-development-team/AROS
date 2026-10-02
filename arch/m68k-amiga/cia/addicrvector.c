/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.

    Desc: AddICRVector() function.
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

    if (iCRBit == -1) {
        /* Hack. Was called by timer.device, see below */
        CiaBase->hook_func = (void(*)(APTR, APTR, WORD))interrupt->is_Code;
        CiaBase->hook_data = interrupt->is_Data;
        return NULL;
    }

    /* 68k lowlevel library calls have garbage in upper word */
    iCRBit = (WORD)iCRBit;
    Disable();
    old = CiaBase->Vectors[iCRBit].iv_Node;
    if (old) {
        /* timer.device move out of our way hack.
         * Check timer.device for details
         */
        if (CiaBase->hook_func) {
            void (*hook)(APTR, APTR, WORD) = CiaBase->hook_func;
            CiaBase->hook_func = NULL;
            hook(CiaBase, CiaBase->hook_data, iCRBit);
            CiaBase->hook_func = hook;
            old = CiaBase->Vectors[iCRBit].iv_Node;
        }
    }
    if (old == NULL) {
        CiaBase->Vectors[iCRBit].iv_Data = interrupt->is_Data;
        CiaBase->Vectors[iCRBit].iv_Code = interrupt->is_Code;
        CiaBase->Vectors[iCRBit].iv_Node = interrupt;
        AbleICR(resource, 0x80 | (1 << iCRBit));
    }
    Enable();
    return old;

    AROS_LIBFUNC_EXIT
}
