/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
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

/*  FUNCTION
        returns the currently pressed 'qualifier' and 'key' combination.

    INPUTS
        none

    RESULT
        0xFF if no key is pressed otherwise it returns the actual key in the low word,
        and qualifier in the high word -:
        
        'qualifier'     key equivalent
	LLKB_LSHIFT     Left Shift
	LLKB_RSHIFT     Rigt Shift
	LLKB_CAPSLOCK   Caps Lock
	LLKB_CONTROL    Control
	LLKB_LALT       Left Alt
	LLKB_RALT       Right Alt
	LLKB_LAMIGA     Left Amiga
	LLKB_RAMIGA     Right Amiga

    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return LowLevelBase->ll_LastKey;

    AROS_LIBFUNC_EXIT
} /* GetKey */
