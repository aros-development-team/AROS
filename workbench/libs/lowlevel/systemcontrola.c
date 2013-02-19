/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#define DEBUG 1
#include <aros/debug.h>
#include <proto/arossupport.h>

#include <aros/libcall.h>
#include <exec/types.h>
#include <libraries/lowlevel.h>

#include "lowlevel_intern.h"

/*****************************************************************************

    NAME */
#include <proto/lowlevel.h>

      AROS_LH1(ULONG, SystemControlA,

/*  SYNOPSIS */ 
      AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 12, LowLevel)

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

    struct TagItem *tag, *tagp = tags;;

    aros_print_not_implemented ("lowlevel/SystemControlA");

    /* For now, dump all tags in debug mode */
    while ((tag = LibNextTagItem(&tagp))) {
        D(bug("%s: Tag SCON_Dummy+%d, Data %p\n", __func__, tag->ti_Tag - SCON_Dummy, (APTR)tag->ti_Data));
    }

    return (tags ? tags->ti_Tag : 0);

    AROS_LIBFUNC_EXIT
} /* SystemControlA */
