/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"
#include <proto/exec.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <dos/dos.h>

/*****************************************************************************

    NAME */
#include <dos/dosasl.h>
#include <proto/dos.h>

        AROS_LH2(LONG, MatchFirst,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR       , pat, D1),
        AROS_LHA(struct AnchorPath *, AP , D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 137, Dos)

/*  FUNCTION
        Searches for the first file or directory that matches a given pattern.
        MatchFirst() initializes the AnchorPath structure for you but you
        must initilize the following fields: ap_Flags, ap_Strlen, ap_BreakBits
        and ap_FoundBreak. The first call to MatchFirst() also passes you
        the first matching file, which you can examine in ap_Info, and
        the directory the file is in, in ap_Current->an_Lock. After the first
        call to MatchFirst(), call MatchNext(). The search begins wherever the
        current directory is set to (see CurrentDir()). For more info on
        patterns, see ParsePattern().

    INPUTS
        pat - pattern to search for
        AP  - pointer to (initilized) AnchorPath structure

    RESULT
        0     = success
        other = DOS error code

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        MatchNext(), MatchEnd(), ParsePattern(), Examine(), CurrentDir()
        <dos/dosasl.h>

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct AChain       *ac;
    LONG                error;

    AP->ap_Flags   = 0;
    AP->ap_Base    = 0;
    AP->ap_Current = 0;

    error = Match_BuildAChainList(pat, AP, &ac, DOSBase);
    if (error == 0)
    {
        AP->ap_Base = AP->ap_Current = ac;

        error = MatchNext(AP);

    } /* if (error == 0) */

    SetIoErr(error);

    return error;

    AROS_LIBFUNC_EXIT
    
} /* MatchFirst */
