/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DosGetString() - Support for localized strings.
    Lang: english
*/

#include "dos_intern.h"

/*****i***********************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(STRPTR, DosGetString,

/*  SYNOPSIS */
        AROS_LHA(LONG, stringNum, D1),

/* LOCATION */
        struct DosLibrary *, DOSBase, 163, Dos)

/*  FUNCTION
        Internal DOS function, will return the string corresponding to
        the number stringNum. 

    INPUTS
        stringNum - The number of the string you want.

    RESULT
        A pointer to a string, or NULL if no string could be found with
        a matching number.

    NOTES
        Error strings will ALWAYS be less than 80 characters, and should
        ideally be less than 60 characters.

        This is a private function, whose the only purpose is to be patched
        by locale.library.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        This is dosPrivate5()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG *p = DOSBase->dl_Errors->estr_Nums;
    UBYTE *q = DOSBase->dl_Errors->estr_Strings;

    do
    {
        LONG n = p[0];
        LONG m = p[1];

        while (n <= m)
        {
            if (n == stringNum)
                return q + 1;

            q += q[0] + 1;
            ++n;
        }

        p += 2;
    }
    while (p[0] != 0);

    return "undefined error";

    AROS_LIBFUNC_EXIT
    
} /* DosGetString */
