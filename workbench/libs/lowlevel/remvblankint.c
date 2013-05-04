/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/
#include "lowlevel_intern.h"

#include <aros/libcall.h>
#include <exec/types.h>
#include <libraries/lowlevel.h>
#include <hardware/intbits.h>

/*****************************************************************************

    NAME */

      AROS_LH1(VOID, RemVBlankInt,

/*  SYNOPSIS */ 
      AROS_LHA(APTR, intHandle, A1),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 19, LowLevel)

/*  FUNCTION

    Remove a vertical blank interrupt routine previously added by a call to
    AddVBlankInt().
 
    INPUTS

    intHandle  --  return value from AddVBlankInt(); may be NULL in which case
                   this function is a no-op.
 
    RESULT
 
    BUGS

    SEE ALSO

    AddVBlankInt()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Protection against erroneous programs */
    if (intHandle != NULL ||
	((struct Interrupt *)intHandle) != &LowLevelBase->ll_VBlank)
    {
	return;
    }
    
    ObtainSemaphore(&LowLevelBase->ll_Lock);
    RemIntServer(INTB_VERTB, &LowLevelBase->ll_VBlank);
    LowLevelBase->ll_VBlank.is_Code = NULL;
    LowLevelBase->ll_VBlank.is_Data = NULL;
    ReleaseSemaphore(&LowLevelBase->ll_Lock);
    
    AROS_LIBFUNC_EXIT
} /* RemVBlankInt */
