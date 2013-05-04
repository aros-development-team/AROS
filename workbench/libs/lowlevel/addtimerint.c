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

      AROS_LH2(APTR, AddTimerInt,

/*  SYNOPSIS */ 
      AROS_LHA(APTR, intRoutine, A0),
      AROS_LHA(APTR, intData, A1),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 13, LowLevel)

/*  FUNCTION
 
    INPUTS
 
    RESULT
 
    BUGS
        This function is unimplemented.

    INTERNALS

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

    /* TODO: Write lowlevel/AddTimerInt() */
    aros_print_not_implemented ("lowlevel/AddTimerInt");

    return NULL; // return failure until implemented

  AROS_LIBFUNC_EXIT
} /* AddTimerInt */
