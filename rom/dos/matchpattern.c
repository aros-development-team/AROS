/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/dosasl.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(BOOL, MatchPattern,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, pat, D1),
        AROS_LHA(CONST_STRPTR, str, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 141, Dos)

/*  FUNCTION
        Check if a string matches a pattern. The pattern must be a pattern as
        output by ParsePattern(). Note that this routine is case sensitive.

    INPUTS
        pat - Pattern string (as returned by ParsePattern())
        str - The string to match against the pattern 'pat'

    RESULT
        Boolean telling whether the string matched the pattern.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        ParsePattern(), MatchPatternNoCase(), MatchFirst(), MatchNext()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return patternMatch(pat, str, TRUE, DOSBase);
        
    AROS_LIBFUNC_EXIT
} /* MatchPattern */
