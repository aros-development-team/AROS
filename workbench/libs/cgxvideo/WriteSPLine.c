/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/
#include <aros/debug.h>

#include "cgxvideo_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

        AROS_LH5(ULONG, WriteSPLine,

/*  SYNOPSIS */
        AROS_LHA(struct VLayerHandle *, VLayerHandle, A0),
        AROS_LHA(UBYTE *, Buffer, A1),
        AROS_LHA(LONG, X, D0),
        AROS_LHA(LONG, Y, D1),
        AROS_LHA(LONG, Width, D2),

/*  LOCATION */
        struct Library *, CGXVideoBase, 17, Cgxvideo)

/*  FUNCTION
        Writes one line of subpicture data to a video layer.

    INPUTS
        VLayerHandle - pointer to a previously created videolayer handle

        Buffer - subpicture pixel data

        X, Y - position of the line in the subpicture

        Width - number of pixels to write

    RESULT
        result - 0

    NOTES
        Available since V50.

    EXAMPLE

    BUGS
        No backend supports subpictures yet, so this does nothing.

    SEE ALSO
        QueryVLayerAttr()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
} /* WriteSPLine */
