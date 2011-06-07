/*
    Copyright 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosmesa_funcs.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

      AROS_LH2(void, AROSMesaSetRast,

/*  SYNOPSIS */ 
      AROS_LHA(AROSMesaContext, amesa, A0),
      AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
      struct Library *, MesaBase, 12, Mesa)

/*  FUNCTION

        Sets a new rendering target for an existing context
 
    INPUTS

        tagList - a pointer to tags to be used during creation.
 
    TAGS

        AMA_Window - pointer to Window onto which scene is to be rendered. Must
                     be provided.

    RESULT

        None
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    if (amesa)
    {
    }

    AROS_LIBFUNC_EXIT
}
