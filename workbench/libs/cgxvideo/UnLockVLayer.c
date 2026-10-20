/*
    Copyright (C) 1995-2010, The AROS Development Team. All rights reserved.
*/
#include <aros/debug.h>
#include <proto/exec.h>

#include "cgxvideo_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

        AROS_LH1(ULONG, UnlockVLayer,

/*  SYNOPSIS */
        AROS_LHA(struct VLayerHandle *, VLayerHandle, A0),

/*  LOCATION */
        struct Library *, CGXVideoBase, 11, Cgxvideo)

/*  FUNCTION
        Unlocks a previouly locked video layer

    INPUTS
        VLayerHandle - pointer to a previously created videolayer handle

    RESULT
        result - always 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        LockVLayer()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (VLayerHandle)
    {
        ObtainSemaphore(&VLayerHandle->lock);
        VLayerHandle->locked = FALSE;
        ReleaseSemaphore(&VLayerHandle->lock);
    }

    return 0;

    AROS_LIBFUNC_EXIT
} /* UnlockVLayer */
