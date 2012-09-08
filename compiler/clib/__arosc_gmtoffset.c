/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Internal arossstdc function to get current GMT offset
*/
#include <proto/exec.h>
#include <proto/locale.h>
#include <exec/execbase.h>

/*****************************************************************************

    NAME */
#include <time.h>

	int __arosc_gmtoffset (

/*  SYNOPSIS */
        void)

/*  FUNCTION

    INPUTS

    RESULT
        The offset to GMT in minutes

    NOTES
        Will return 0 when locale.library is not loaded into memory yet.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        Will always query the current locale through locale.library to
        get the GMT offset.

******************************************************************************/
{
    static struct LocaleBase *LocaleBase = NULL;
    struct Locale *loc;
    int gmtoffset = 0;

    if (!LocaleBase)
    {
        struct Node *found;

        /* Only open locale.library if it does not have to be loaded from disk */
        Forbid();
        found = FindName(&SysBase->LibList, "locale.library");
        Permit();

        if (found)
            LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 0);
    }

    if (LocaleBase && (loc = OpenLocale(NULL)))
    {
        gmtoffset = (int)loc->loc_GMTOffset;
        CloseLocale(loc);
    }

    return gmtoffset;
}
