/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: AmigaGuide function ReplyAmigaGuideMsg()
    Lang: english
*/
#include "amigaguide_intern.h"

/*****************************************************************************

    NAME */
#include <proto/amigaguide.h>

        AROS_LH1(void, ReplyAmigaGuideMsg,

/*  SYNOPSIS */
        AROS_LHA(struct AmigaGuideMsg *, msg, A0),

/*  LOCATION */
        struct Library *, AmigaGuideBase, 14, AmigaGuide)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, AmigaGuideBase)

#warning TODO: Write amigaguide/ReplyAmigaGuideMsg()
    aros_print_not_implemented ("amigaguide/ReplyAmigaGuideMsg");

    AROS_LIBFUNC_EXIT
} /* ReplyAmigaGuideMsg */
