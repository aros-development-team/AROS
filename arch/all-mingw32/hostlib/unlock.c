#include <aros/libcall.h>
#include <proto/exec.h>

#include "hostlib_intern.h"

AROS_LH0I(void, HostLib_Unlock,
	 struct HostLibBase *, HostLibBase, 8, HostLib)
{
    AROS_LIBFUNC_INIT

    Permit();

    AROS_LIBFUNC_EXIT
}
