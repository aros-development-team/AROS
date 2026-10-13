/*
    Copyright (C) 1995-2007, The AROS Development Team. All rights reserved.

*/

#include "intuition_intern.h"
#include "inputhandler.h"

/*****************************************************************************

    NAME */
        AROS_LH4(ULONG, SetPointerBounds,
/*  SYNOPSIS */
         AROS_LHA(struct Screen    *, screen  , A0),
         AROS_LHA(struct Rectangle *, rect    , A1),
         AROS_LHA(ULONG             , reserved, D0),
         AROS_LHA(struct TagItem   *, tags    , A2),
/*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 160, Intuition)

/*  FUNCTION
        Set, refresh or clear a temporary global pointer constraint.

    INPUTS
        screen - Ignored by this implementation; may be NULL.
        rect - Inclusive bounds in Intuition's global display-coordinate
               system, or NULL to clear an active temporary constraint.
               MinX must not exceed MaxX; MinY must not exceed MaxY.
        reserved - Pass zero; currently ignored.
        tags - Currently ignored; pass NULL.

    RESULT
        Always 0, including when the request is ignored. The return value
        does not indicate whether the bounds were installed or removed.

    NOTES
        A non-NULL rectangle is copied when no constraint is active, or
        when the existing constraint has a nonzero expiry counter. Each
        accepted update resets that counter to five IECLASS_TIMER events.
        The input device normally schedules these events at 100 ms intervals;
        this is roughly half a second, not a guaranteed one-second lifetime.
        Event phase and processing delays affect the elapsed time.

        Passing NULL clears an active constraint only while its expiry
        counter is nonzero. An active constraint with a zero counter is
        left unchanged by either form of this call.

        The rectangle is borrowed only for this call and is never modified.
        The bounds apply to subsequent input processing; the call does not
        immediately reposition the pointer. Screen origin and scaling are
        not applied. Keep bounds within the current display's usable area.

        The constraint is global, not owned by a task, window or screen.
        Another caller or Intuition's window drag/resize handling may change
        or clear it. Call from task context where waiting is permitted;
        the function acquires the input-handler semaphore.

    BUGS
        The screen and tag parameters have no effect. Rectangle ordering
        and display limits are not validated. No failure or ownership status
        is provided, and an active constraint with a zero expiry counter
        cannot be replaced or cleared through this function.

    SEE ALSO
        SetPointer(), SetWindowPointerA()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IIHData *iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
   
    (void)screen;
    (void)reserved;
    
    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->InputHandlerLock);
    if (!iihd->MouseBoundsActiveFlag)
    {
        if (rect)
        {
            iihd->MouseBoundsActiveFlag = TRUE;
            iihd->MouseBoundsKillTimer  = 5; /* Five input timer events. */
            iihd->MouseBoundsLeft       = rect->MinX;
            iihd->MouseBoundsTop        = rect->MinY;
            iihd->MouseBoundsRight      = rect->MaxX;
            iihd->MouseBoundsBottom     = rect->MaxY;
        }
    }
    else if (iihd->MouseBoundsKillTimer)
    {
        if (rect)
        {
            /* Reset timer */
            iihd->MouseBoundsKillTimer  = 5; /* Five input timer events. */
            iihd->MouseBoundsLeft       = rect->MinX;
            iihd->MouseBoundsTop        = rect->MinY;
            iihd->MouseBoundsRight      = rect->MaxX;
            iihd->MouseBoundsBottom     = rect->MaxY;
        }
        else
        {
            iihd->MouseBoundsKillTimer = 0;
            iihd->MouseBoundsActiveFlag = FALSE;
        }
    }
    
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->InputHandlerLock);
    
    return 0;
    
    AROS_LIBFUNC_EXIT
}
