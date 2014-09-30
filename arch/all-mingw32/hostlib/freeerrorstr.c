/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <proto/exec.h>

#include "../kernel/hostinterface.h"
#include "hostlib_intern.h"

AROS_LH1(void, HostLib_FreeErrorStr,
	 AROS_LHA(char *, error, A0),
	 struct HostLibBase *, HostLibBase, 4, HostLib)
{
    AROS_LIBFUNC_INIT

    Forbid();
    HostLibBase->HostIFace->hostlib_FreeErrorStr(error);
    Permit();

    AROS_LIBFUNC_EXIT
}
