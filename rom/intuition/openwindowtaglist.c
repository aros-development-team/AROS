/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/utility.h>
#include "intuition_intern.h"
#include <utility/tagitem.h>
#include <proto/exec.h>

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH2(struct Window *, OpenWindowTagList,

         /*  SYNOPSIS */
         AROS_LHA(struct NewWindow *, newWindow, A0),
         AROS_LHA(struct TagItem   *, tagList, A1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 101, Intuition)

/*  FUNCTION
 
    INPUTS
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
    29-10-95    digulla automatically created from
                intuition_lib.fd and clib/intuition_protos.h
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct ExtNewWindow  nw =
    {
        0, 0,       	/* Left, Top */
        ~0, ~0,     	/* Width, Height */
        0xFF, 0xFF, 	/* DetailPen, BlockPen */
        0L,         	/* IDCMPFlags */
        0L,         	/* Flags */
        NULL,       	/* FirstGadget */
        NULL,       	/* CheckMark */
        NULL,       	/* Title */
        NULL,       	/* Screen */
        NULL,       	/* BitMap */
        0, 0,       	/* MinWidth, MinHeight */
        0, 0,       	/* MaxWidth, MaxHeight */
        WBENCHSCREEN,   /* Type */
        NULL        	/* Extension (taglist) */
    };
    struct Window       *window;

    DEBUG_OPENWINDOWTAGLIST(dprintf("OpenWindowTagList: NewWindow 0x%lx TagList 0x%lx\n",
                                    newWindow, tagList));

    if (newWindow)
    {
        ASSERT_VALID_PTR_ROMOK(newWindow);

        CopyMem (newWindow, &nw, (newWindow->Flags & WFLG_NW_EXTENDED) ? sizeof (struct ExtNewWindow) :
                 sizeof (struct NewWindow));

        if (tagList)
        {
            ASSERT_VALID_PTR_ROMOK(tagList);

            nw.Extension = tagList;
            nw.Flags |= WFLG_NW_EXTENDED;
        }
    }
    else
    {
        struct TagItem tags[2] =
        {
            {WA_AutoAdjust  ,  TRUE 	},
            {TAG_END	    ,       NULL}
        };

        nw.Extension = tags;
        nw.Flags |= WFLG_NW_EXTENDED;

        if (tagList)
        {
            ASSERT_VALID_PTR_ROMOK(tagList);

            tags[1].ti_Tag = TAG_MORE;
            tags[1].ti_Data = (IPTR)tagList;
        }
    }


    window = OpenWindow ((struct NewWindow *)&nw);

    return window;

    AROS_LIBFUNC_EXIT

} /* OpenWindowTagList */
