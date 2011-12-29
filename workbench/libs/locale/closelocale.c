/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CloseLocale() - Close a locale structure.
    Lang: english
*/
#include <exec/types.h>
#include <proto/exec.h>
#include "locale_intern.h"

/*****************************************************************************

    NAME */
#include <proto/locale.h>

        AROS_LH1(void, CloseLocale,

/*  SYNOPSIS */
        AROS_LHA(struct Locale *, locale, A0),

/*  LOCATION */
        struct LocaleBase *, LocaleBase, 7, Locale)

/*  FUNCTION
        Finish accessing a Locale.

    INPUTS
        locale  -   An opened locale. Note that NULL is a valid
                    parameter here, and will simply be ignored.

    RESULT
        The locale is released back to the system.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        OpenLocale()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Best make sure we actually have something freeable. */
    if (locale
        && (locale != (struct Locale *)IntLB(LocaleBase)->lb_DefaultLocale))
    {
        /* Make sure we don't have any race conditions */
        ObtainSemaphore(&IntLB(LocaleBase)->lb_LocaleLock);
        if (--IntL(locale)->il_Count == 0)
        {
            /* Free the locale structure if it's not the current locale */
            if (locale !=
                (struct Locale *)IntLB(LocaleBase)->lb_CurrentLocale)
            {
                /* Close old .language, if any */
                if (IntLB(locale)->lb_CurrentLocale->il_CurrentLanguage)
                    CloseLibrary(IntLB(locale)->lb_CurrentLocale->
                        il_CurrentLanguage);

                /* Close old dos.catalog */
                CloseCatalog(IntLB(locale)->lb_CurrentLocale->
                    il_DosCatalog);

                FreeMem(locale, sizeof(struct IntLocale));
            }
        }
        ReleaseSemaphore(&IntLB(LocaleBase)->lb_LocaleLock);
    }

    AROS_LIBFUNC_EXIT
}
