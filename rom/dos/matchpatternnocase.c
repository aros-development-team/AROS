/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <dos/dosextens.h>
#include <dos/dosasl.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(BOOL, MatchPatternNoCase,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, pat, D1),
        AROS_LHA(CONST_STRPTR, str, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 162, Dos)

/*  FUNCTION
        Similar to MatchPattern(), only case insensitive (see there for
        more information). For use with ParsePatternNoCase().

    INPUTS
        pat - Pattern as returned by ParsePatternNoCase()
        str - String to match against the pattern 'pat'

    RESULT
        Boolean telling whether the match was successful or not.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        MatchPattern(), ParsePatternNoCase()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return patternMatch(pat, str, FALSE, DOSBase);

    AROS_LIBFUNC_EXIT
} /* MatchPatternNoCase */
