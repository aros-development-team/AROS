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

/*****************************************************************************

    NAME */

      AROS_LH1(VOID, RemKBInt,

/*  SYNOPSIS */ 
      AROS_LHA(APTR, intHandle, A1),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 11, LowLevel)

/*  FUNCTION
           remove a keyboard interrupt previously registerd
           with addkbint.

    INPUTS
 
    RESULT
 
    BUGS

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

    if (intHandle)
    {
        struct Interrupt *kbInt = (struct Interrupt *)intHandle;
        ObtainSemaphore(&LowLevelBase->ll_Lock);
        Remove(&kbInt->is_Node);
        ReleaseSemaphore(&LowLevelBase->ll_Lock);
        FreeVec(kbInt);
    }

    return;

  AROS_LIBFUNC_EXIT
} /* RemKBInt */
