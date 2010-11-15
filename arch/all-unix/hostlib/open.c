#define DEBUG 0
#include <aros/debug.h>
#include <aros/libcall.h>

#include "../kernel/hostinterface.h"

#include "hostlib_intern.h"

AROS_LH2(void *, HostLib_Open,
         AROS_LHA(const char *, filename, A0),
         AROS_LHA(char **,      error,    A1),
         struct HostLibBase *, HostLibBase, 1, HostLib)
{

    AROS_LIBFUNC_INIT

    AROS_HOST_BARRIER
    return HostLibBase->HostIFace->HostLib_Open(filename, error);

    AROS_LIBFUNC_EXIT
}
