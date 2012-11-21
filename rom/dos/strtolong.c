/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Convert a string into a long
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2I(LONG, StrToLong,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, string, D1),
        AROS_LHA(LONG *, value,  D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 136, Dos)

/*  FUNCTION
        Convert a string to a long number.

    INPUTS
        string - The value to convert
        value - The result is returned here

    RESULT
        How many characters in the string were considered when it was
        converted or -1 if no valid number could be found.

    NOTES
        The routine doesn't check if the number if too large.

    EXAMPLE
        // Valid number are: 5, -1, +3, +0007, etc.

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG sign=0, v=0;
    CONST_STRPTR s=string;

    /* Skip leading whitespace characters */
    while(*s==' '||*s=='\t')
        s++;

    /* Swallow sign */
    if(*s=='+'||*s=='-')
        sign=*s++;

    /* If there is no number return an error. */
    if(*s<'0'||*s>'9')
    {
        *value=0;
        return -1;
    }

    /* Calculate result */
    do
        v=v*10+*s++-'0';
    while(*s>='0'&&*s<='9');

    /* Negative? */
    if(sign=='-')
        v=-v;

    /* All done. */
    *value=v;
    return s-string;
    AROS_LIBFUNC_EXIT
} /* StrToLong */
