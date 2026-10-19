/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/
#include <aros/debug.h>
#include <proto/exec.h>

#include "cgxvideo_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

        AROS_LH1(void, DeleteVLayerHandle,

/*  SYNOPSIS */
        AROS_LHA(struct VLayerHandle *, VLayerHandle, A0),

/*  LOCATION */
        struct Library *, CGXVideoBase, 6, Cgxvideo)

/*  FUNCTION
        Deletes a created video layer handle

    INPUTS
        VLayerHandle - pointer to a previously created videolayer handle

    RESULT
        none

    NOTES
        The handle may still be locked or attached.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!VLayerHandle)
        return;

    FreeVec(VLayerHandle->buffer);
    FreeMem(VLayerHandle, sizeof(struct VLayerHandle));

    AROS_LIBFUNC_EXIT
} /* DeleteVLayerHandle */
