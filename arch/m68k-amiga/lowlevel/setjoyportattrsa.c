/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include <aros/debug.h>

#include <libraries/lowlevel.h>

/*****************************************************************************

    NAME */

      AROS_LH2(BOOL, SetJoyPortAttrsA,

/*  SYNOPSIS */ 
      AROS_LHA(ULONG, portNumber, D0),
      AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 22, LowLevel)

/*  FUNCTION
 
    INPUTS
 
    RESULT
 
    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct TagItem *ti;

    while ((ti = LibNextTagItem(&tagList)) != NULL) {
        D(bug("%s: Tag %d = %p\n", __func__, ti->ti_Tag, (APTR)ti->ti_Data));
    }
    return FALSE;
    
    AROS_LIBFUNC_EXIT
    
} /* SetJoyPortAttrsA */
