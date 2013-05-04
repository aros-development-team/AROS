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

      AROS_LH2(APTR, AddVBlankInt,

/*  SYNOPSIS */ 
      AROS_LHA(APTR, intRoutine, A0),
      AROS_LHA(APTR, intData, A1),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 18, LowLevel)

/*  FUNCTION

    Add a callback function that should be executed every vertical blank.
    If your program can exit without rebooting the machine, RemVBlankInt()
    has to be called prior to exiting.
        Only one interrupt routine may be added; always check the return
    value of this function in case some other program already has used this
    function.

    INPUTS

    intRoutine  --  the callback function to invoke each vertical blank
    intData     --  data passed to the callback function
 
    RESULT

    A handle used to manipulate the interrupt or NULL if the call failed.
 
    BUGS

    SEE ALSO

    RemVBlankInt()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    APTR result;

    if (intRoutine == NULL)
    {
	return NULL;
    }

    ObtainSemaphore(&LowLevelBase->ll_Lock);

    if (LowLevelBase->ll_VBlank.is_Code == NULL)
    {
	LowLevelBase->ll_VBlank.is_Code = intRoutine;
	LowLevelBase->ll_VBlank.is_Data = intData;

	AddIntServer(INTB_VERTB, &LowLevelBase->ll_VBlank);
	result = (APTR)&LowLevelBase->ll_VBlank;
    }
    else
    {
	result = NULL;
    }

    ReleaseSemaphore(&LowLevelBase->ll_Lock);

    return result;

    AROS_LIBFUNC_EXIT
} /* AddVBlankInt */
