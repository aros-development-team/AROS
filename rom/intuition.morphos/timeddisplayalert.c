/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$
 
    Desc: Intuition function TimedDisplayAlert()
    Lang: english
*/
#include "intuition_intern.h"

#ifdef __MORPHOS__
#include "morphos/displayalert.h"
#endif

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH4(BOOL, TimedDisplayAlert,

         /*  SYNOPSIS */
         AROS_LHA(ULONG  , alertnumber, D0),
         AROS_LHA(UBYTE *, string     , A0),
         AROS_LHA(UWORD  , height     , D1),
         AROS_LHA(ULONG  , time       , A1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 137, Intuition)

/*  FUNCTION
 
    INPUTS
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#ifdef __MORPHOS__

    return int_TimedDisplayAlert(alertnumber, string, height, time, IntuitionBase);

#else

#warning TODO: Write intuition/TimedDisplayAlert()
    aros_print_not_implemented ("TimedDisplayAlert");

    /* shut up the compiler */
    IntuitionBase = IntuitionBase;
        alertnumber = alertnumber;
        string = string;
        height = height;
        time = time;

    return FALSE;

#endif
    AROS_LIBFUNC_EXIT
} /* TimedDisplayAlert */
