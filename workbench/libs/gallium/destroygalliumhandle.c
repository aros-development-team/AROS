/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "gallium_intern.h"

/*****************************************************************************

    NAME */

      AROS_LH1(void, DestroyGalliumHandle,

/*  SYNOPSIS */ 
      AROS_LHA(struct GalliumHandle *, handle, A0),

/*  LOCATION */
      struct Library *, GalliumBase, 6, Gallium)

/*  NAME
 
    FUNCTION
    Frees resources associated with the handle.
 
    INPUTS
    handle - a pointer to GalliumHandle structure. A NULL pointer will be
           ignored.
 
    RESULT
    The resources associated with handle are freed and should no longer be
    used.
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    if (handle)
    {
        if (handle->PipeScreen)
        {
            handle->PipeScreen->destroy(handle->PipeScreen);
        }
        
        FreeVec(handle);
    }
    
    AROS_LIBFUNC_EXIT
}
