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

      AROS_LH1(VOID, StopTimerInt,

/*  SYNOPSIS */ 
      AROS_LHA(APTR , intHandle, A1),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 15, LowLevel)

/*  FUNCTION
 
    INPUTS
 
    RESULT
 
    BUGS
        This function is unimplemented.

    INTERNALS

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

    /* TODO: Write lowlevel/StopTimerInt() */
    aros_print_not_implemented ("lowlevel/StopTimerInt");

    return;

  AROS_LIBFUNC_EXIT
} /* StopTimerInt */
