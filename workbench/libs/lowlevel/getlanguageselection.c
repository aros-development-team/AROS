/*
    (C) 1995-2000 AROS - The Amiga Research OS
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

      AROS_LH0(ULONG, GetLanguageSelection,

/*  SYNOPSIS */ 

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 11, LowLevel)

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

#warning TODO: Write lowlevel/GetLanguageSelection()
    aros_print_not_implemented ("lowlevel/GetLanguageSelection");

    return 0L; // return "no language has been selected" until implemented

  AROS_LIBFUNC_EXIT
} /* GetLanguageSelection */
