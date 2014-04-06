#include <exec/types.h>
#include "vg_api.h"

/*****************************************************************************

    NAME */

      APTR GetOpenVGStateTrackerApi()

/*  SYNOPSIS */ 

/*  FUNCTION
        This is a PRIVATE function used by egl.library to receive pointer to
        api structure of OpenVG. Do not use this function in your application.
        OpenVG context is created using EGL API.
 
    INPUTS
 
    RESULT
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    return (APTR)vg_api_get();
}
