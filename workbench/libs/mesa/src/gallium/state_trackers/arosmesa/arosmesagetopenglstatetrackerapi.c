#include "state_tracker/st_gl_api.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

      AROS_LH0(APTR, AROSMesaGetOpenGLStateTrackerApi,

/*  SYNOPSIS */ 

/*  LOCATION */
      struct Library *, MesaBase, 0, Mesa)

/*  FUNCTION
        This is a PRIVATE function used by egl.library to receive pointer to
        api structure of OpenGL. Do not use this function in your application.
 
    INPUTS
 
    RESULT
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (APTR)st_gl_api_create();

    AROS_LIBFUNC_EXIT
}
