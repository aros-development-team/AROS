
#include <exec/libraries.h>
#include <aros/asmcall.h>
#include <proto/cia.h>
#include <hardware/cia.h>

#include "cia_intern.h"

AROS_UFH4(APTR, Cia_Handler,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(void *, data, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, mySysBase, A6))
{ 
    AROS_USERFUNC_INIT

    struct CIABase *CiaBase = (struct CIABase *)data;

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
                    AROS_UFC4NR(void, ciaint->is_Code,
                              AROS_UFCA(ULONG, dummy, A0),
                              AROS_UFCA(APTR, ciaint->is_Data, A1),
                              AROS_UFCA(APTR, ciaint->is_Code, A5),
                              AROS_UFCA(struct ExecBase *, mySysBase, A6)
                              );
                }
            }
        }
        CiaBase->executing_mask = 0;
    }

    return 0;

    AROS_USERFUNC_EXIT
}
