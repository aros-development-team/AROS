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
        AROS_LH2(void *, HostLib_Open,

/*  SYNOPSIS */
         AROS_LHA(const char *, filename, A0),
         AROS_LHA(char **,      error,    A1),

/*  LOCATION */
         struct HostLibBase *, HostLibBase, 1, HostLib)
/*  FUNCTION
        Open a shared library through the host operating system's dynamic
        loader. This function is provided by hostlib.resource on hosted
        AROS systems; it does not open an AROS library.

    INPUTS
        filename - Non-NULL pointer to a NUL-terminated host library name
                   or path. Name lookup and supported file formats are
                   determined by the host loader.
        error - Optional pointer to a char * for a host diagnostic, or NULL
                if no diagnostic is wanted. Initialize *error to NULL before
                the call.

    RESULT
        A host library handle on success, or NULL on failure. Balance each
        successful open with HostLib_Close() after all uses of the library
        and its symbols have ended.

    NOTES
        The filename is borrowed only for this call. A returned diagnostic
        uses host-specific storage: use or copy it before another host
        loader operation can replace it, and release a non-NULL string
        with HostLib_FreeErrorStr(). Do not use FreeVec() or free(). A
        failure need not have an available diagnostic. Determine success
        from the returned handle, not from the diagnostic pointer.

        On Unix hosts this uses dlopen() with RTLD_NOW | RTLD_GLOBAL.
        On Windows hosts it uses LoadLibrary(). The Unix wrapper does not
        clear an earlier loader error before opening, so a diagnostic can
        refer to an earlier operation even when this open succeeds.

        Hostlib serializes loader calls. Loading may run host initialization
        code. Call host library functions using the host ABI and any
        required host-call synchronization.

    SEE ALSO
        HostLib_Close(), HostLib_GetPointer(), HostLib_FreeErrorStr(),
        HostLib_Lock(), HostLib_Unlock()

*****************************************************************************/
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
