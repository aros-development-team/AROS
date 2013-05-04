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

      AROS_LH0(ULONG, GetKey,

/*  SYNOPSIS */ 

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 8, LowLevel)

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

    /* TODO: Write lowlevel/GetKey() */
    aros_print_not_implemented ("lowlevel/GetKey");

    return 0xFF; // return "no key pressed" until implemented

  AROS_LIBFUNC_EXIT
} /* GetKey */
