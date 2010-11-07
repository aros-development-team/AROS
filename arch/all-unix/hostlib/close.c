#include <aros/debug.h>
#include <aros/libcall.h>

#include "../kernel/hostinterface.h"

#include "hostlib_intern.h"

AROS_LH2(int, HostLib_Close,
         AROS_LHA(void *,  handle, A0),
         AROS_LHA(char **, error,  A1),
         struct HostLibBase *, HostLibBase, 2, HostLib)
{
    AROS_LIBFUNC_INIT

    return HostLibBase->HostIFace->HostLib_Close(handle, error);

    AROS_LIBFUNC_EXIT
}
