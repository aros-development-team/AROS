/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Convert a string into a long
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
        The routine doesn't check if the number is too large.
        Examples of valid number: 5, -1, +3, +0007, etc.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG sign=0;
    ULONG v=0;
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
        v=v*10+(ULONG)(*s++-'0');
    while(*s>='0'&&*s<='9');

    /* Negative? */
    if(sign=='-')
        v=(ULONG)0-v;

    /* All done. */
    /* Accumulate modulo ULONG, including the magnitude of LONG_MIN. Form
       a negative LONG without overflowing or an out-of-range signed cast.
       As before, numbers that are too large are not rejected. */
    if(v > (~(ULONG)0 >> 1))
        *value=-1-(LONG)~v;
    else
        *value=(LONG)v;
    return s-string;
    AROS_LIBFUNC_EXIT
} /* StrToLong */
