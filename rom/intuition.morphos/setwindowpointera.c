/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/graphics.h>
#include <proto/utility.h>
#include "intuition_intern.h"
#include <intuition/pointerclass.h>

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH2(void, SetWindowPointerA,

         /*  SYNOPSIS */
         AROS_LHA(struct Window * , window , A0),
         AROS_LHA(struct TagItem *, taglist, A1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 136, Intuition)

/*  FUNCTION
 
    INPUTS
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    DEBUG_SETPOINTER(dprintf("SetWindowPointer: window 0x%lx\n", window));

    if (window)
    {
        ULONG 	lock;
        Object *pointer = (Object *)GetTagData(WA_Pointer, NULL, taglist);
        BOOL	busy = (GetTagData(WA_BusyPointer, FALSE, taglist) != 0) ? TRUE : FALSE;

        DEBUG_SETPOINTER(dprintf("SetWindowPointer: pointer 0x%lx busy %d\n",
                                 pointer, busy));

        lock = LockIBase(0);

        DEBUG_SETPOINTER(dprintf("SetWindowPointer: old pointer 0x%lx busy %d\n",
                                 IW(window)->pointer, IW(window)->busy));

        if (IW(window)->pointer != pointer || IW(window)->busy != busy)
        {
            Object *oldpointer = NULL;

            if (IW(window)->free_pointer)
                oldpointer = IW(window)->pointer;

            IW(window)->pointer = pointer;
            IW(window)->busy = busy;
            IW(window)->free_pointer = FALSE;
            window->Pointer = NULL;

            if (window->Flags & WFLG_WINDOWACTIVE)
            {
                struct IntScreen *scr = GetPrivScreen(window->WScreen);

                if (GetTagData(WA_PointerDelay, FALSE, taglist) &&
                        IntuitionBase->ActiveScreen == &scr->Screen)
                {
                    DEBUG_SETPOINTER(dprintf("SetWindowPointer: delay change\n"));
                    GetPrivIBase(IntuitionBase)->PointerDelay = 5;
                }
                else
                {
                    struct SharedPointer *shared_pointer;

                    if (pointer == NULL)
                        pointer = GetPrivIBase(IntuitionBase)->DefaultPointer;
                    if (busy)
                        pointer = GetPrivIBase(IntuitionBase)->BusyPointer;

                    GetAttr(POINTERA_SharedPointer, pointer, (IPTR *)&shared_pointer);

                    DEBUG_POINTER(dprintf("SetWindowPointer: scr 0x%lx pointer 0x%lx sprite 0x%lx\n",
                                          scr, pointer, shared_pointer->sprite));

                    if (ChangeExtSpriteA(&scr->Screen.ViewPort,
                                         scr->Pointer->sprite, shared_pointer->sprite, NULL))
                    {
                        ObtainSharedPointer(shared_pointer, IntuitionBase);
                        ReleaseSharedPointer(scr->Pointer, IntuitionBase);
                        scr->Pointer = shared_pointer;
                        if (window)
                        {
                            window->XOffset = shared_pointer->xoffset;
                            window->YOffset = shared_pointer->yoffset;
                        }
                    }
                    else
                    {
                        DEBUG_POINTER(dprintf("SetWindowPointer: can't set pointer.\n"));
                    }
                }
            }

            DisposeObject(oldpointer);
        }

        UnlockIBase(lock);
    }

    AROS_LIBFUNC_EXIT
} /* SetWindowPointerA */
