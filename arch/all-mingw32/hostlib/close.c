#include <proto/exec.h>

#include <stdarg.h>

#include "../kernel/hostinterface.h"
#include "hostlib_intern.h"

AROS_LH2(BOOL, HostLib_Close,
         AROS_LHA(void *,  handle, A0),
         AROS_LHA(char **, error,  A1),
         struct HostLibBase *, HostLibBase, 2, HostLib)
{
    AROS_LIBFUNC_INIT

    BOOL ret;

    Forbid();
    ret = HostLibBase->HostIFace->HostLib_Close(handle, error);
    Permit();

    return ret;

    AROS_LIBFUNC_EXIT
}
