/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
      struct LowLevelBase *, LowLevelBase, 13, LowLevel)

/*  NAME
 
    FUNCTION
 
    INPUTS
 
    RESULT
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct LowLevelBase *, LowLevelBase)

#warning TODO: Write lowlevel/GetKey()
    aros_print_not_implemented ("lowlevel/GetKey");

    return 0xFF; // return "no key pressed" until implemented

  AROS_LIBFUNC_EXIT
} /* GetKey */
