/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"
#include <intuition/pointerclass.h>

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH6(void, SetPointer,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),
         AROS_LHA(UWORD         *, pointer, A1),
         AROS_LHA(LONG           , height, D0),
         AROS_LHA(LONG           , width, D1),
         AROS_LHA(LONG           , xOffset, D2),
         AROS_LHA(LONG           , yOffset, D3),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 45, Intuition)

/*  FUNCTION
    Changes the shape of the mouse pointer for a given window.
 
    INPUTS
    window - Change it for this window
    pointer - The shape of the new pointer as a bitmap with depth 2.
    height - Height of the pointer
    width - Width of the pointer (must be <= 16)
    xOffset, yOffset - The offset of the "hot spot" relative to the
        left, top edge of the bitmap.
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    ClearPointer()
 
    INTERNALS
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    EXTENDWORD(height);
    EXTENDWORD(width);
    EXTENDWORD(xOffset);
    EXTENDWORD(yOffset);

    DEBUG_SETPOINTER(dprintf("SetPointer: window 0x%lx pointer data 0x%lx\n",
                             window, pointer));

    if (window && pointer)
    {
        Object *ptrobj = MakePointerFromData(IntuitionBase,
                                             pointer + 2, xOffset, yOffset, width, height);

        if (ptrobj)
        {
            struct TagItem pointertags[] =
            {
                {WA_Pointer, (IPTR)ptrobj},
                {TAG_DONE                }
            };

            window->Pointer = pointer;
            window->PtrWidth = width;
            window->PtrHeight = height;

            SetWindowPointerA(window, pointertags);

            IW(window)->free_pointer = TRUE;
        }
    }

    AROS_LIBFUNC_EXIT
} /* SetPointer */
