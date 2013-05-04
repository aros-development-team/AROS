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

      AROS_LH2(VOID, QueryKeys,

/*  SYNOPSIS */ 
      AROS_LHA(struct KeyQuery *, queryArray	, A0),
      AROS_LHA(UBYTE		, arraySize	, D1),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 9, LowLevel)

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

    /* TODO: Write lowlevel/QueryKeys() */
    aros_print_not_implemented ("lowlevel/QueryKeys");

    return;

  AROS_LIBFUNC_EXIT
} /* QueryKeys */
