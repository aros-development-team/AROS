/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/hostlib.h>

#include <stdarg.h>

#include "hostinterface.h"
#include "hostlib_intern.h"

AROS_LH2(void *, HostLib_Open,
         AROS_LHA(const char *, filename, A0),
         AROS_LHA(char **,      error,    A1),
         struct HostLibBase *, HostLibBase, 1, HostLib)
{
    AROS_LIBFUNC_INIT

    void *ret;

    HOSTLIB_LOCK();

    ret = HostLibBase->HostIFace->hostlib_Open(filename, error);
    AROS_HOST_BARRIER

    HOSTLIB_UNLOCK();

    return ret;

    AROS_LIBFUNC_EXIT
}
