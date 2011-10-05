#include "arosmesa_types.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

      AROS_LH1(APTR, AROSMesaGetProcAddressInternal,

/*  SYNOPSIS */
      AROS_LHA(const GLubyte *, procname, A0),

/*  LOCATION */
      struct Library *, MesaBase, 14, Mesa)

/*  FUNCTION
        This is a PRIVATE function used by AROSMesaGetProcAddress function
        to validate if the requested function is actually supported by
        current configuration of 3D subsystem. Do not use this function 
        in your application.
 
    INPUTS
 
    RESULT
        The return valud is defined as APTR. If it is not NULL, the requested
        GL function is supported by current configuration of 3D subsystem.
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return glstapi->get_proc_address(glstapi, procname);

    AROS_LIBFUNC_EXIT
}
