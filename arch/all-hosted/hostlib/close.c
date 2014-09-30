/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

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

    HOSTLIB_LOCK();

    ret = HostLibBase->HostIFace->hostlib_Close(handle, error);
    AROS_HOST_BARRIER

    HOSTLIB_UNLOCK();

    return ret;

    AROS_LIBFUNC_EXIT
}
