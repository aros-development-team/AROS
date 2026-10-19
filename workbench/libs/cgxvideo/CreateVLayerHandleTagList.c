/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include "cgxvideo_intern.h"

#define setError(x) if (errPtr) *errPtr = x

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

        AROS_LH2(struct VLayerHandle *, CreateVLayerHandleTagList,

/*  SYNOPSIS */
        AROS_LHA(struct Screen *, Screen, A0),
        AROS_LHA(struct TagItem  *, TagItems, A1),

/*  LOCATION */
        struct Library *, CGXVideoBase, 5, Cgxvideo)

/*  FUNCTION
        Creates a video layer handle for the given screen

    INPUTS
        Screen - Screen we wish to create a handle for

        TagItems - pointer to an optional tag list

    RESULT
        VLayerHandle - pointer to the created videolayer handle or 0

    NOTES
        Tags available are:

                VOA_SrcType (ULONG) - specifies source type that is used for video
                              overlay data

                        Currently supported formats:

                                SRCFMT_YUV16 (not recommended, use YCbCr16 instead)
                                SRCFMT_YCbCr16
                                SRCFMT_RGB15PC
                                SRCFMT_RGB16PC

                VOA_SrcWidth (ULONG) - source width in pixel units

                VOA_SrcHeight (ULONG) -  source height in pixel units

                VOA_Error (ULONG *) - If you specify VOA_Error with ti_Data pointing
                              to an ULONG, you will get more detailed information
                              if the creation of the video layer handle fails

                VOA_UseColorKey (BOOL) - If you specify VOA_UseColorKey as TRUE, color
                        keying is enabled for the video layer. A
                        certain color key is generated then and the
                        stream data is only visible where this color
                        could be found.

                VOA_UseBackFill (BOOL) - If you specify VOA_UseBackFill as TRUE automatic
                        backfilling for the videolayer is enabled. This
                        option is only available if color keying is
                        enabled.

                VOA_Modulo (ULONG) - bytes per source row (V43). Defaults to
                        VOA_SrcWidth * 2.

        Error codes:

                VOERR_INVSRCFMT - unknown VOA_SrcType, zero dimensions or a
                        modulo shorter than a source row

                VOERR_INVSCRMODE - the screen has no video overlay

                VOERR_NOMEMORY - out of memory

    EXAMPLE

    BUGS
        No display driver provides an overlay yet. Setting the
        CGXVideoNullBackend variable selects a backend that keeps the
        source in system RAM and displays nothing. It does not accept
        SRCFMT_YCbCr420.

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct VLayerHandle *vh;
    ULONG *errPtr = (ULONG *)GetTagData(VOA_Error, 0, TagItems);
    ULONG srctype = GetTagData(VOA_SrcType, SRCFMT_YUV16, TagItems);
    ULONG width   = GetTagData(VOA_SrcWidth , 0, TagItems);
    ULONG height  = GetTagData(VOA_SrcHeight, 0, TagItems);
    ULONG modulo  = GetTagData(VOA_Modulo, width * 2, TagItems);

    if (srctype > SRCFMT_RGB16PC || !width || !height || modulo < width * 2)
    {
        setError(VOERR_INVSRCFMT);
        return NULL;
    }

    if (!Screen || !cgxv_NullBackend())
    {
        setError(VOERR_INVSCRMODE);
        return NULL;
    }

    vh = AllocMem(sizeof(struct VLayerHandle), MEMF_ANY | MEMF_CLEAR);
    if (vh)
        vh->buffer = AllocVec(modulo * height, MEMF_ANY | MEMF_CLEAR);
    if (!vh || !vh->buffer)
    {
        if (vh)
            FreeMem(vh, sizeof(struct VLayerHandle));
        setError(VOERR_NOMEMORY);
        return NULL;
    }

    InitSemaphore(&vh->lock);
    vh->width  = width;
    vh->height = height;
    vh->modulo = modulo;
    setError(VOERR_OK);

    return vh;

    AROS_LIBFUNC_EXIT
} /* CreateVLayerHandleTagList */
