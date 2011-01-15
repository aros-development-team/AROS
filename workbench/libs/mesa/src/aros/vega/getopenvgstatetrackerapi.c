#include "vega/openvg_intern.h"
#include "vg_api.h"

/*****************************************************************************

    NAME */

      AROS_LH0(APTR, GetOpenVGStateTrackerApi,

/*  SYNOPSIS */ 

/*  LOCATION */
      struct Library *, OpenVGBase, 0, Openvg)

/*  FUNCTION
        This is a private function used by mesa.library to receive pointer to
        api structure of OpenVG. Do not use this function in your application.
        OpenVG context is created using EGL API.
 
    INPUTS
 
    RESULT
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (APTR)vg_api_get();

    AROS_LIBFUNC_EXIT
}
