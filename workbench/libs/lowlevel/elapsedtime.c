/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#include "lowlevel_intern.h"

#include <aros/libcall.h>
#include <exec/types.h>
#include <devices/timer.h>
#include <libraries/lowlevel.h>

/*****************************************************************************

    NAME */

      AROS_LH1(ULONG, ElapsedTime,

/*  SYNOPSIS */ 
      AROS_LHA(struct EClockVal *, context, A0),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 17, LowLevel)

/*  FUNCTION
 
    INPUTS
 
    RESULT
 
    BUGS
        This function is unimplemented.

    INTERNALS

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

    /* TODO: Write lowlevel/ElapsedTime() */
    aros_print_not_implemented ("lowlevel/ElapsedTime");

    return 0L;

  AROS_LIBFUNC_EXIT
} /* ElapsedTime */
