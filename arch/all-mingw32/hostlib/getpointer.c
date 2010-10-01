#include <proto/exec.h>

#include <stdarg.h>

#include "../kernel/hostinterface.h"
#include "hostlib_intern.h"

AROS_LH3(void *, HostLib_GetPointer,
         AROS_LHA(void *,       handle, A0),
         AROS_LHA(const char *, symbol, A1),
         AROS_LHA(char **,      error,  A2),
         struct HostLibBase *, HostLibBase, 3, HostLib)
{
    AROS_LIBFUNC_INIT

    void *ret;

    Forbid();
    ret = HostLibBase->HostIFace->HostLib_GetPointer(handle, symbol, error);
    Permit();

    return ret;

    AROS_LIBFUNC_EXIT
}
