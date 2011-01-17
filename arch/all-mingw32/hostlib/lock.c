#include <aros/libcall.h>
#include <proto/exec.h>

#include "hostlib_intern.h"

AROS_LH0I(void, HostLib_Lock,
	 struct HostLibBase *, HostLibBase, 7, HostLib)
{
    AROS_LIBFUNC_INIT

    Forbid();

    AROS_LIBFUNC_EXIT
}
