/*
    Copyright (C) 1995-2010, The AROS Development Team. All rights reserved.
*/
#include <aros/debug.h>
#include <proto/exec.h>

#include "cgxvideo_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

        AROS_LH1(ULONG, LockVLayer,

/*  SYNOPSIS */
        AROS_LHA(struct VLayerHandle *, VLayerHandle, A0),

/*  LOCATION */
        struct Library *, CGXVideoBase, 10, Cgxvideo)

/*  FUNCTION
        Locks the specified video layer to allow access to source data. Make
        sure that you don't keep that lock for too long. It is only allowed
        to keep it for a short time.

    INPUTS
        VLayerHandle - pointer to a previously created videolayer handle

    RESULT
        result - TRUE if video layer could be locked, FALSE otherwise

    NOTES

    EXAMPLE

    BUGS
        The lock does not nest; a second LockVLayer() fails until
        UnlockVLayer() is called.

    SEE ALSO
        UnlockVLayer()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG result = FALSE;

    if (!VLayerHandle)
        return result;

    ObtainSemaphore(&VLayerHandle->lock);
    if (!VLayerHandle->locked)
    {
        VLayerHandle->locked = TRUE;
        result = TRUE;
    }
    ReleaseSemaphore(&VLayerHandle->lock);

    return result;

    AROS_LIBFUNC_EXIT
} /* LockVLayer */
