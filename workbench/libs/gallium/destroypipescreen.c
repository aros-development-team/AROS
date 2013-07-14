/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "gallium_intern.h"

/*****************************************************************************

    NAME */

      AROS_LH1(void, DestroyPipeScreen,

/*  SYNOPSIS */ 
      AROS_LHA(struct pipe_screen *, screen, A0),

/*  LOCATION */
      struct Library *, GalliumBase, 6, Gallium)

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
    
    if (screen)
    {
        screen->destroy(screen);
    }
    
    AROS_LIBFUNC_EXIT
}
