/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
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

      AROS_LH1(ULONG, ReadJoyPort,

/*  SYNOPSIS */ 
      AROS_LHA(ULONG, port, D0),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 5, LowLevel)

/*  FUNCTION
 
    INPUTS
 
    RESULT
 
    NOTES
        This function isn't implemented on all platforms.

    BUGS

    INTERNALS

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

    /* TODO: Write lowlevel/ReadJoyPort() */
    // aros_print_not_implemented ("lowlevel/ReadJoyPort");

    return JP_TYPE_NOTAVAIL; // return failure until implemented

  AROS_LIBFUNC_EXIT
} /* ReadJoyPort */
