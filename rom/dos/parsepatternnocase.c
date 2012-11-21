/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <proto/exec.h>
#include <proto/utility.h>
#include <dos/dos.h>
#include <dos/dosasl.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH3(LONG, ParsePatternNoCase,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, Source,     D1),
        AROS_LHA(STRPTR,       Dest,       D2),
        AROS_LHA(LONG,         DestLength, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 161, Dos)

/*  FUNCTION
        Similar to ParsePattern(), only case insensitive (see there
        for more information). For use with MatchPatternNoCase().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        ParsePattern(), MatchPatternNoCase()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return patternParse(Source, Dest, DestLength, FALSE, DOSBase);

    AROS_LIBFUNC_EXIT
} /* ParsePatternNoCase */
