/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/
#include <aros/debug.h>

#include "cgxvideo_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

        AROS_LH2(ULONG, QueryVLayerAttr,

/*  SYNOPSIS */
        AROS_LHA(struct Screen *, Screen, A0),
        AROS_LHA(ULONG, AttrID, D0),

/*  LOCATION */
        struct Library *, CGXVideoBase, 18, Cgxvideo)

/*  FUNCTION
        Queries the video layer capabilities of a screen.

    INPUTS
        Screen - screen to query

        AttrID - one of:

                VSQ_SupportedFeatures - mask of VSQ_FEAT_... bits

                VSQ_SupportedFormats - mask of VSQ_FMT_... bits

                VSQ_MaxWidth - maximum source width of an overlay

                VSQ_MaxWidthSP - maximum width of a subpicture

    RESULT
        value - the value of the attribute, 0 if unsupported

    NOTES
        Available since V50.

    EXAMPLE

    BUGS
        Only the null backend reports any capabilities.

    SEE ALSO
        CreateVLayerHandleTagList()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!Screen || !cgxv_NullBackend())
        return 0;

    switch (AttrID)
    {
    case VSQ_SupportedFeatures:
        return VSQ_FEAT_OVERLAY;

    case VSQ_SupportedFormats:
        return VSQ_FMT_YUYV | VSQ_FMT_R5G5B5_LE | VSQ_FMT_R5G6B5_LE;

    case VSQ_MaxWidth:
        return 0xffff;
    }

    return 0;

    AROS_LIBFUNC_EXIT
} /* QueryVLayerAttr */
