/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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

      AROS_LH1(VOID, RemTimerInt,

/*  SYNOPSIS */ 
      AROS_LHA(APTR, intHandle, A1),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 14, LowLevel)

/*  NAME
 
    FUNCTION
 
    INPUTS
 
    RESULT
 
    BUGS
        This function is unimplemented.

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

    /* TODO: Write lowlevel/RemTimerInt() */
    aros_print_not_implemented ("lowlevel/RemTimerInt");

    if (intHandle)
    {
    }

    return;

  AROS_LIBFUNC_EXIT
} /* RemTimerInt */
