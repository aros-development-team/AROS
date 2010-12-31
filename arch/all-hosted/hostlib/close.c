#include <proto/exec.h>
#include <proto/hostlib.h>

#include <stdarg.h>

#include "hostinterface.h"
#include "hostlib_intern.h"

AROS_LH2(int, HostLib_Close,
         AROS_LHA(void *,  handle, A0),
         AROS_LHA(char **, error,  A1),
         struct HostLibBase *, HostLibBase, 2, HostLib)
{
    AROS_LIBFUNC_INIT

    int ret;

    HostLib_Lock();

    ret = HostLibBase->HostIFace->hostlib_Close(handle, error);
    AROS_HOST_BARRIER

    HostLib_Unlock();

    return ret;

    AROS_LIBFUNC_EXIT
}
