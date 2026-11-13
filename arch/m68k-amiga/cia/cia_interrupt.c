/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/libraries.h>
#include <aros/asmcall.h>
#include <proto/cia.h>
#include <hardware/cia.h>

#include "cia_intern.h"

AROS_INTH1(Cia_Handler, struct CIABase *, CiaBase)
{
    AROS_INTFUNC_INIT

    UBYTE mask;
     
    CiaBase->active_mask |= CiaBase->hw->ciaicr & 0x1f;
    mask = CiaBase->enable_mask & CiaBase->active_mask;
     
    if (mask) {
        int i;
        CiaBase->executing_mask = mask;
        CiaBase->active_mask &= ~mask;
        for (i = 0; i < VECTORS_NUM; i++) {
            if (mask & (1 << i)) {
                struct CIAIntVector *iv = &CiaBase->Vectors[i];
                if (iv->iv_Code) {
                    AROS_INTC1(iv->iv_Code, iv->iv_Data);
                }
            }
        }
        CiaBase->executing_mask = 0;
    }

    return FALSE;

    AROS_INTFUNC_EXIT
}
