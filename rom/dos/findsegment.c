/*
    Copyright (C) 1995-2016, The AROS Development Team. All rights reserved.

    Desc: Find a resident segment.
*/

#include <aros/debug.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <dos/dosextens.h>
#include <proto/dos.h>

        AROS_LH3(struct Segment *, FindSegment,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR    , name, D1),
        AROS_LHA(struct Segment *, seg, D2),
        AROS_LHA(LONG            , system, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 130, Dos)

/*  FUNCTION
        Search for a resident segment by name and type (system or user).
        The first segment that exactly matches the name and type will be
        returned. The name is case insensitive. If the system argument is
        non-zero, only system segments will be returned (i.e. those that
        have a negative seg_UC value); if zero, only user segments will
        be returned (i.e. those with a non-negative seg_UC value).

        You can continue searching for multiple segments that share the
        same name and type by specifying the last returned segment as
        the seg argument.

        FindSegment() does no locking of the segment list. You should
        lock the list by calling Forbid() before calling FindSegment(),
        and unlock the list by calling Permit() once you have finished
        calling FindSegment().

        If you wish to prevent a user segment from being unloaded, you
        must increment its seg_UC value before unlocking the list. Once
        finished with the segment, you must decrement its seg_UC value
        under Forbid()/Permit() protection. The seg_UC value of system
        segments should never be altered.

    INPUTS
        name - Name of the segment to search for.
        seg  - Start search from this point.
        system - Search for a system segment.

    RESULT
        A matching segment, or NULL.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        AddSegment(), RemSegment()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct DosInfo *dinf = BADDR(DOSBase->dl_Root->rn_Info);

    /* Segment seg was the last match, let's start from the next one */
    if( seg != NULL )
        seg = BADDR(seg->seg_Next);
    else
        seg = BADDR(dinf->di_ResList);

    while( seg != NULL )
    {
        D(bug("[FindSegment] Checking segment '%s'\n",
            AROS_BSTR_ADDR(MKBADDR(&seg->seg_Name[0]))));
        if
        (
            ((system && seg->seg_UC < 0) || (!system && seg->seg_UC >= 0)) &&
            (Stricmp( name, AROS_BSTR_ADDR(MKBADDR(&seg->seg_Name[0]))) == 0)
        )
        {
                /* We have a matching segment */
                return seg;
        }
        seg = BADDR(seg->seg_Next);
    }

    return NULL;
    AROS_LIBFUNC_EXIT
} /* FindSegment */
