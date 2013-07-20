#include <proto/exec.h>
#include <proto/hostlib.h>

#include <stdarg.h>

#include "hostinterface.h"
#include "hostlib_intern.h"

/*****************************************************************************

    NAME */

    AROS_LH3(void *, HostLib_GetPointer,

/*  SYNOPSIS */
    AROS_LHA(void *,       handle, A0),
    AROS_LHA(const char *, symbol, A1),
    AROS_LHA(char **,      error,  A2),

/*  LOCATION */
    struct HostLibBase *, HostLibBase, 3, HostLib)
    
/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    void *ret;

    HOSTLIB_LOCK();

    ret = HostLibBase->HostIFace->hostlib_GetPointer(handle, symbol, error);
    AROS_HOST_BARRIER

    HOSTLIB_UNLOCK();

    return ret;

    AROS_LIBFUNC_EXIT
}
