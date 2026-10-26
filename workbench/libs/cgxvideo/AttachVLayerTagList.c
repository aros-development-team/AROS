/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/
#include <aros/debug.h>
#include <proto/exec.h>

#include "cgxvideo_intern.h"


/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

        AROS_LH3(ULONG, AttachVLayerTagList,

/*  SYNOPSIS */
        AROS_LHA(struct VLayerHandle *, VLayerHandle, A0),
        AROS_LHA(struct Window *, Window, A1),
        AROS_LHA(struct TagItem  *, TagItems, A2),

/*  LOCATION */
        struct Library *, CGXVideoBase, 7, Cgxvideo)

/*  FUNCTION
        Attaches a previously created videolayer handle to the specified window.
        The video overlay should now be dynamically linked to the window.
        If the window is moved or resized, the overlay is also moved or resized.

    INPUTS
        VLayerHandle - pointer to a previously created videolayer handle

        Window - pointer to the window that the videolayer should be attached to

        TagItems - pointer to an optional tag list

    RESULT
        result - 0 if videolayer could be attached to the window

    NOTES
        Tags available are:

                VOA_LeftIndent (ULONG) - additional offset from the left window border

                VOA_RightIndent (ULONG) - additional offset from the right window border

                VOA_TopIndent (ULONG) - additional offset from the top window border

                VOA_BottomIndent (ULONG) - additional offset from the bottom window border

        The source is scaled to the window's inner rectangle minus the
        indents. With VOA_UseColorKey the rectangle is filled with the
        key colour, and with VOA_UseBackfill a layer backfill hook keeps
        it filled.

        Detach the layer before the window is closed. Should a window
        close with a layer still attached, the library detaches it.

    EXAMPLE

    BUGS
        Fails while the layer is locked or already attached, when the
        window is not on the handle's screen, or when the screen is not
        shown on its display.

    SEE ALSO
        DetachVLayer()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IntCGXVBase *base = GetCGXVBase(CGXVideoBase);
    struct VLayerHandle *vh = VLayerHandle;
    ULONG result = TRUE;

    if (!vh || !Window)
        return result;

    ObtainSemaphore(&base->handleLock);
    ObtainSemaphore(&vh->lock);
    if (!vh->locked && !(vh->flags & VHF_ATTACHED))
        result = cgxv_Attach(vh, Window, TagItems);
    ReleaseSemaphore(&vh->lock);
    ReleaseSemaphore(&base->handleLock);

    return result;

    AROS_LIBFUNC_EXIT
} /* AttachVLayerTagList */
