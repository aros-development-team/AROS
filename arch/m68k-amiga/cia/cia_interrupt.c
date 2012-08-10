
#include <exec/libraries.h>
#include <aros/asmcall.h>
#include <proto/cia.h>
#include <hardware/cia.h>

#include "cia_intern.h"

AROS_UFIH1(Cia_Handler, struct CIABase *, CiaBase)
{ 
    AROS_USERFUNC_INIT

    UBYTE mask;
     
    CiaBase->active_mask |= CiaBase->hw->ciaicr & 0x1f;
    mask = CiaBase->enable_mask & CiaBase->active_mask;
     
    if (mask) {
        int i;
        CiaBase->executing_mask = mask;
        CiaBase->active_mask &= ~mask;
        for (i = 0; i < VECTORS_NUM; i++) {
            if (mask & (1 << i)) {
                struct Interrupt *ciaint = CiaBase->Vectors[i];
                if (ciaint) {
                    AROS_UFIC1(ciaint->is_Code, ciaint->is_Data);
                }
            }
        }
        CiaBase->executing_mask = 0;
    }

    return FALSE;

    AROS_USERFUNC_EXIT
}
