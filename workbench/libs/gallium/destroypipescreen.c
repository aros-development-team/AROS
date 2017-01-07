/*
    Copyright 2010-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "gallium_intern.h"

/*****************************************************************************

    NAME */

      AROS_LH2(void, DestroyPipeScreen,

/*  SYNOPSIS */ 
      AROS_LHA(PipeHandle_t, pipe, A0),
      AROS_LHA(struct pipe_screen *, pscreen, A1),

/*  LOCATION */
      struct Library *, GalliumBase, 8, Gallium)

/*  FUNCTION
        Disposes the pipe screen
 
    INPUTS
        handle - a pointer to pipe screen structure. A NULL pointer will be
            ignored.
 
    RESULT
        The pipe screen is freed. Don't use it anymore.
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    if (pscreen)
    {
        pscreen->destroy(pscreen);
    }
    
    AROS_LIBFUNC_EXIT
}
