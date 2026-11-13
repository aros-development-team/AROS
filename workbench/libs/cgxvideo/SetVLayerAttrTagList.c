/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "cgxvideo_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

        AROS_LH2(void, SetVLayerAttrTagList,

/*  SYNOPSIS */
        AROS_LHA(struct VLayerHandle *, VLayerHandle, A0),
        AROS_LHA(struct TagItem  *, TagItems, A1),

/*  LOCATION */
        struct Library *, CGXVideoBase, 12, Cgxvideo)

/*  FUNCTION
        Sets certain attributes for a given video layer

    INPUTS
        VLayerHandle - pointer to a previously created videolayer handle

        TagItems - pointer to a tag list which contains attributes to be
                modified

    RESULT
        none

    NOTES
        Tags available are:

                VOA_LeftIndent (ULONG) - additional offset from the left window
                                                border

                VOA_RightIndent (ULONG) - additional offset from the right window
                                                border

                VOA_TopIndent (ULONG) - additional offset from the top window border

                VOA_BottomIndent (ULONG) - additional offset from the bottom window
                                                border

                VOA_ColorSpace (ULONG) - AROS extension: VOCS_BT601,
                                                VOCS_BT709 or VOCS_BT2020

                VOA_ColorRange (ULONG) - AROS extension: VOCR_LIMITED or
                                                VOCR_FULL

        A changed indent moves the overlay at once; a changed colour space
        or range applies from the next frame, for a stream that changes its
        encoding. Subpicture and colour key tags are ignored.

    EXAMPLE

    BUGS

    SEE ALSO
        GetVLayerAttr()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct VLayerHandle *vh = VLayerHandle;
    struct TagItem *tag, *tstate = TagItems;
    BOOL changed = FALSE;

    if (!vh)
        return;

    ObtainSemaphore(&vh->lock);

    while ((tag = NextTagItem(&tstate)))
    {
        LONG idx;

        switch (tag->ti_Tag)
        {
        case VOA_LeftIndent:   idx = 0; break;
        case VOA_TopIndent:    idx = 1; break;
        case VOA_RightIndent:  idx = 2; break;
        case VOA_BottomIndent: idx = 3; break;
        case VOA_ColorSpace:
            /* VOCS_* are vHidd_ColorSpace_* */
            if (!(vh->flags & VHF_NULL))
                OOP_SetAttrsTags(vh->bm, aHidd_BitMap_ColorSpace, tag->ti_Data, TAG_DONE);
            idx = -1;
            break;
        case VOA_ColorRange:
            /* VOCR_* are vHidd_ColorRange_* */
            if (!(vh->flags & VHF_NULL))
                OOP_SetAttrsTags(vh->bm, aHidd_BitMap_ColorRange, tag->ti_Data, TAG_DONE);
            idx = -1;
            break;
        default:               idx = -1; break;
        }

        if (idx >= 0 && vh->indents[idx] != (LONG)tag->ti_Data)
        {
            vh->indents[idx] = tag->ti_Data;
            changed = TRUE;
        }
    }

    if (changed)
        st_SetStale(&vh->track);
    cgxv_Refresh(vh);

    ReleaseSemaphore(&vh->lock);

    AROS_LIBFUNC_EXIT
} /* SetVLayerAttrTagList */
