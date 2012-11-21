/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosasl.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH3(LONG, ParsePattern,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, Source,     D1),
        AROS_LHA(STRPTR, Dest,       D2),
        AROS_LHA(LONG,   DestLength, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 140, Dos)

/*  FUNCTION
        Takes a pattern containing wildcards and transforms it into some
        intermediate representation for use with the MatchPattern() function.
        The intermediate representation is longer but generally a buffer
        size of 2*(strlen(Source)+1) is enough. Nevertheless you should check
        the returncode to be sure that everything went fine.

    INPUTS
        Source     - Pattern describing the kind of strings that match.
                     Possible tokens are:
                     #x     - The following character or item is repeaded 0 or
                              more times.
                     ?      - Item matching a single non-NUL character.
                     a|b|c  - Matches one of multiple strings.
                     ~x     - This item matches if the item x doesn't match.
                     (a)    - Parens
                     [a-z]  - Matches a single character out of the set.
                     [~a-z] - Matches a single non-NUL character not in the set.
                     'c     - Escapes the following character.
                     *      - Same as #?, but optional.
        Dest       - Buffer for the destination.
        DestLength - Size of the buffer.

    RESULT
         1 - There are wildcards in the pattern (it might match more than
             one string).
         0 - No wildcards in it, all went fine.
        -1 - An error happened. IoErr() gives additional information in
             that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return patternParse(Source, Dest, DestLength, TRUE, DOSBase);

    AROS_LIBFUNC_EXIT
} /* ParsePattern */
