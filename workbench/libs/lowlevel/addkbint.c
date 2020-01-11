/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#include "lowlevel_intern.h"

#include <aros/libcall.h>
#include <exec/types.h>
#include <libraries/lowlevel.h>

AROS_UFH1(void, KBEventWrapper,
          AROS_UFHA(struct llKBInterrupt *,      kbInt,       A0))
{
    AROS_USERFUNC_INIT

    D(
        bug("[lowlevel] %s()\n", __func__);
        bug("[lowlevel] %s: Calling func @ 0x%p\n", __func__, kbInt->llkbi_Code);
        bug("[lowlevel] %s: Data = %p\n", __func__, kbInt->llkbi_Data);
        bug("[lowlevel] %s: key data =  %08x\n", __func__, kbInt->llkbi_KeyData);
    )

  AROS_UFC3(void, kbInt->llkbi_Code,
      AROS_UFCA(ULONG, kbInt->llkbi_KeyData, D0),
      AROS_UFCA(APTR, kbInt->llkbi_Data, A1),
      AROS_UFCA(APTR, kbInt->llkbi_Code, A5));

    AROS_USERFUNC_EXIT
}

/*****************************************************************************

    NAME */

      AROS_LH2(APTR, AddKBInt,

/*  SYNOPSIS */ 
      AROS_LHA(APTR, intRoutine, A0),
      AROS_LHA(APTR, intData, A1),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 10, LowLevel)

/*  FUNCTION
           register a callback that is called whenever a keyboard
           input event occurs.

    INPUTS
 	intRoutine - the routine to invoke every vblank. This routine should
		     be as short as possible to minimize its effect on overall
		     system performance.
	intData - data passed to the routine in register A1. If more than one
		  long word of data is required this should be a pointer to
		  a structure that contains the required data.

    RESULT
 
    BUGS

    INTERNALS

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

    struct llKBInterrupt *kbInt = (struct llKBInterrupt *)AllocVec(sizeof(struct llKBInterrupt), MEMF_CLEAR);
    if (kbInt)
    {
        kbInt->llkbi_Interrupt.is_Code = KBEventWrapper;
        kbInt->llkbi_Code = intRoutine;
        kbInt->llkbi_Data = intData;
        kbInt->llkbi_Interrupt.is_Data = kbInt;
        AddTail(&LowLevelBase->ll_KBInterrupts, &kbInt->llkbi_Interrupt.is_Node);
        return kbInt;
    }

    return NULL; // return failure until implemented

  AROS_LIBFUNC_EXIT
} /* AddKBInt */
