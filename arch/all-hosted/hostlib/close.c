/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/hostlib.h>

#include <stdarg.h>

#include "hostinterface.h"
#include "hostlib_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH2(int, HostLib_Close,

/*  SYNOPSIS */
         AROS_LHA(void *,  handle, A0),
         AROS_LHA(char **, error,  A1),

/*  LOCATION */
         struct HostLibBase *, HostLibBase, 2, HostLib)
/*  FUNCTION
        Release a host loader reference acquired by HostLib_Open(). The
        host loader decides when the shared library can be unloaded and
        may execute its termination code during this call.

    INPUTS
        handle - A valid handle returned by a successful HostLib_Open()
                 whose corresponding reference has not yet been closed.
        error - Optional pointer to a char * for a host diagnostic, or NULL
                if no diagnostic is wanted. Initialize *error to NULL before
                the call.

    RESULT
        Zero on success; nonzero if the host loader reports failure.
        Do not assume a particular nonzero error value across hosts.

    NOTES
        Finish using the library and its symbols before closing it. A
        successful close invalidates this reference; symbol pointers may
        also become invalid. Concurrent users must coordinate separately.

        A returned diagnostic uses host-specific storage. Use or copy it
        before another host loader operation can replace it, and release
        a non-NULL string with HostLib_FreeErrorStr(), not FreeVec() or
        free(). Failure need not produce a diagnostic. Use the return
        value to determine success: the Unix wrapper does not clear an
        earlier loader error before calling dlclose().

        Windows returns the negation of FreeLibrary()'s success value;
        Unix returns dlclose()'s result. Hostlib serializes loader calls.

    SEE ALSO
        HostLib_Open(), HostLib_GetPointer(), HostLib_FreeErrorStr()

*****************************************************************************/
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
